add_lldb_unittest(DAPTests
  JSONUtilsTest.cpp
  LLDBUtilsTest.cpp
  TransportTest.cpp
  ProtocolTypesTest.cpp

  LINK_LIBS
    lldbDAP
    LLVMTestingSupport
  LINK_COMPONENTS
    Support
  )
