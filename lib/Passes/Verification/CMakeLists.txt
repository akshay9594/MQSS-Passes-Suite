add_mlir_library(
  MQSSCIVerificationPasses
  CommonVerificationPass.cpp
  DEPENDS
  MQSSVerificationPassIncGen
  LINK_LIBS
  PUBLIC
  MQSSSupportedDialects
  MQT::CoreIR
  MLIRPass
  MLIRTransforms)

target_compile_options(MQSSCIVerificationPasses PRIVATE -fexceptions)
if(TARGET obj.MQSSCIVerificationPasses)
  target_compile_options(obj.MQSSCIVerificationPasses PRIVATE -fexceptions)
endif()

target_include_directories(MQSSCIVerificationPasses
                           PRIVATE ${MQT_CORE_INCLUDE_DIR})
