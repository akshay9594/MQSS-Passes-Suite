set(LLVM_TARGET_DEFINITIONS VerificationPasses.td)

mlir_tablegen(VerificationPasses.h.inc -gen-pass-decls -name MQSSVerification)

add_public_tablegen_target(MQSSVerificationPassIncGen)
