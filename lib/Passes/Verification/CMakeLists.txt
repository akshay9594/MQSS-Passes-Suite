add_mlir_library(
  MQSSCIVerificationPasses
  CommonVerificationPass.cpp
  DEPENDS
  MQSSVerificationPassIncGen
  LINK_LIBS
  PUBLIC
  MQSSSupportedDialects
  MQT::CoreIR
  MQT::QCEC
  MLIRPass
  MLIRTransforms)

target_compile_options(MQSSCIVerificationPasses PRIVATE -fexceptions)
if(TARGET obj.MQSSCIVerificationPasses)
  target_compile_options(obj.MQSSCIVerificationPasses PRIVATE -fexceptions)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(MQSSCIVerificationPasses PRIVATE MQSS_ENABLE_DEBUG)
endif()
