add_mlir_library(MQSSCIInterfaces MQSSCompiler.cpp LINK_LIBS PUBLIC
                 MQSSCITransformPasses MQSSCIVerificationPasses)

target_compile_options(MQSSCIInterfaces PRIVATE -fexceptions)
