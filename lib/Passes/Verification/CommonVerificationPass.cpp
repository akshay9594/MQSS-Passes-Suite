

#include "Passes/Verification/VerificationPasses.h"
#include "Utils/DebugUtils.h"
#include "Utils/MQTCoreUtils.h"

namespace mqss::mqssci::verify {

#define GEN_PASS_DEF_COMMONCIRCUITVERIFICATIONPASS
#include "Passes/Verification/VerificationPasses.h.inc"

} // namespace mqss::mqssci::verify

using namespace mlir;
using namespace llvm;

namespace {

struct CommonCircuitVerification
    : public mqss::mqssci::verify::impl::CommonCircuitVerificationPassBase<
          CommonCircuitVerification> {
public:
  void runOnOperation() override {
    auto &selector = getAnalysis<DialectAnalysisSelector>();
    auto &analysis = *selector.get();
    auto DialectTy = selector.getDialect();

    MQSS_DEBUG("\n[Applying Pass: Circuit Verification]\n");

    for (auto [kernel, info] : analysis.getKernelDialectInfo()) {

      MQSS_DEBUG("\nkernel: " << kernel.getSymName() << "\n"
                              << " total input qubits: " << info.AllocatedQubits
                              << " Measure qubits: " << info.NumMeasureQubits
                              << "\n\n");

      if (info.AllocatedQubits == 0)
        continue;

      qc::QuantumComputation qc{info.AllocatedQubits, info.NumMeasureQubits};
      MapVector<mlir::Operation *, int> MeasureOps;
      SmallPtrSet<mlir::Operation *, 16> OpsToErase;
      SmallVector<SmallVector<mlir::Value, 2>> AllResults;

      for (auto &[Op, qview] : info.OpQViewMap) {
        if (qview.GateTy != Gate::UNKNOWN) {
          loadGateOpsIntoQC(Op, qview, qc, qview.isControlled());
          OpsToErase.insert(Op);
        }

        if (qview.isMeasureOp) {
          loadMeasureOpIntoQC(qview, qc);
          MeasureOps[Op] = qview.measurements.size();
        }
      }
    }

    if (failed(analysis.verifyModule())) {
      llvm::errs() << "[" << getArgument() << "]"
                   << " : MLIR Module verification failed\n";
    }
  }
};
} // namespace

std::unique_ptr<mlir::Pass>
mqss::mqssci::verify::CommonCircuitVerificationPass() {
  return std::make_unique<CommonCircuitVerification>();
}
