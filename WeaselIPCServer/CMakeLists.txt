add_library(WeaselIPCServer STATIC SecurityAttribute.cpp WeaselServerImpl.cpp)
target_link_libraries(WeaselIPCServer PUBLIC weasel_headers
  PRIVATE WeaselIPC Boost::thread)
set_target_properties(WeaselIPCServer PROPERTIES FOLDER "Libraries")
