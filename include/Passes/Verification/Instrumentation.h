
#include "EquivalenceCheckingManager.hpp"
#include "Passes/Analysis/Extractor.h"
#include "Utils/DebugUtils.h"
#include "Utils/MQTCoreUtils.h"

#include <cstddef>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <mlir/Pass/PassInstrumentation.h>
#include <string>
#include <unordered_map>
#include <vector>

struct VerifyQuantumComputationTy {

  qc::QuantumComputation qc1;
  qc::QuantumComputation qc2;
};

struct EquivalenceVerificationInstrumentation
    : public mlir::PassInstrumentation {

public:
  explicit EquivalenceVerificationInstrumentation(
      llvm::DenseMap<StringRef, VerifyQuantumComputationTy> snap_shot)
      : cached_module_snapshot(std::move(snap_shot)) {}

  llvm::DenseMap<StringRef, VerifyQuantumComputationTy> get_module_snap_shot() {

    return cached_module_snapshot;
  }

private:
  qc::QuantumComputation
  generateQuantumComputation(std::size_t allocatedQubits,
                             std::size_t numMeasureQubits,
                             MapVector<Operation *, QuantumOpView> OpQView) {

    qc::QuantumComputation qc{allocatedQubits, numMeasureQubits};

    MapVector<mlir::Operation *, int> MeasureOps;
    SmallPtrSet<mlir::Operation *, 16> OpsToErase;
    SmallVector<SmallVector<mlir::Value, 2>> AllResults;

    for (auto &[Op, qview] : OpQView) {
      if (qview.GateTy != Gate::UNKNOWN) {
        loadGateOpsIntoQC(Op, qview, qc, qview.isControlled());
        OpsToErase.insert(Op);
      }

      if (qview.isMeasureOp) {
        loadMeasureOpIntoQC(qview, qc);
        MeasureOps[Op] = qview.measurements.size();
      }
    }

    return qc;
  }

  // Note that the argument "op" here is the MLIR Module
  // Generate and cache module snapshot i.e. DenseMap<llvm::StringRef,
  // qc::QuantumComputation> before the invoked pass.
  void runBeforePass(mlir::Pass *pass, mlir::Operation *op) override {

    MQSS_DEBUG("-->runBeforePass: " << pass->getName() << "\n");

    DialectAnalysisSelector selector(op);
    auto &analysis = *selector.get();
    for (auto [kernel, info] : analysis.getKernelDialectInfo()) {

      MQSS_DEBUG("\nkernel: " << kernel.getSymName() << "\n"
                              << "total input qubits: " << info.AllocatedQubits
                              << " Measure qubits: " << info.NumMeasureQubits
                              << "\n");

      if (info.AllocatedQubits == 0)
        continue;

      auto name = kernel.getSymName();
      VerifyQuantumComputationTy vqc_ty;

      // vqc_ty.qc1 is the quantum circuit before the pass is invoked.
      vqc_ty.qc1 = generateQuantumComputation(
          info.AllocatedQubits, info.NumMeasureQubits, info.OpQViewMap);
      cached_module_snapshot[kernel.getSymName()] = std::move(vqc_ty);
    }
  }

  void runAfterPass(Pass *pass, Operation *op) override {

    MQSS_DEBUG("\n-->runAfterPass: " << pass->getName() << "\n");

    DialectAnalysisSelector selector(op);
    auto &analysis = *selector.get();
    for (auto [kernel, info] : analysis.getKernelDialectInfo()) {

      MQSS_DEBUG("\nkernel: " << kernel.getSymName() << "\n"
                              << "total input qubits: " << info.AllocatedQubits
                              << " Measure qubits: " << info.NumMeasureQubits
                              << "\n");

      if (info.AllocatedQubits == 0)
        continue;

      auto name = kernel.getSymName();
      assert(cached_module_snapshot.count(name) &&
             "instrumentation: function does not exist in cached module "
             "snapshot!");
      auto &vqc_ty = cached_module_snapshot[name];

      // vqc_ty.qc1 is the quantum circuit before the pass is invoked.
      vqc_ty.qc2 = generateQuantumComputation(
          info.AllocatedQubits, info.NumMeasureQubits, info.OpQViewMap);
    }

    // setup default configuration
    ec::Configuration config{};
    config = ec::Configuration{};
    config.functionality.checkPartialEquivalence = true;

    for (auto [func_name, vqc_ty] : cached_module_snapshot) {
      if (vqc_ty.qc1.empty() || vqc_ty.qc2.empty()) {
        MQSS_DEBUG("Skipping verification for: " << func_name
                                                 << " as no snapshots exist\n");
        continue;
      }

      config.execution.runAlternatingChecker = true;
      ec::EquivalenceCheckingManager ecm(vqc_ty.qc1, vqc_ty.qc2, config);

      MQSS_DEBUG("Equivalence check Result for: " << func_name);
      if (ecm.equivalence() ==
          ec::EquivalenceCriterion::EquivalentUpToGlobalPhase) {
        MQSS_DEBUG(" Equivalent\n");
      } else {
        MQSS_DEBUG(" NOT equivalent\n");
      }
    }
  }

  llvm::DenseMap<llvm::StringRef, VerifyQuantumComputationTy>
      cached_module_snapshot;
};
