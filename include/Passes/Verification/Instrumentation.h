
#include <mlir/Pass/PassInstrumentation.h>
class EquivalenceVerificationInstrumentation
    : public mlir::PassInstrumentation {

public:
  void runBeforePass(mlir::Pass *pass, mlir::Operation *op) override {}
};
