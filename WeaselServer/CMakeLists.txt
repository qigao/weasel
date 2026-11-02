add_executable(WeaselServer WIN32
  SystemTraySDK.cpp WeaselServer.cpp WeaselServerApp.cpp WeaselService.cpp
  WeaselTrayIcon.cpp WeaselServer.rc)
target_link_libraries(WeaselServer PRIVATE
  RimeWithWeasel WeaselIPCServer WeaselIPC WinSparkle::WinSparkle Boost::thread imm32)
target_include_directories(WeaselServer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_options(WeaselServer PRIVATE /MANIFEST:EMBED
  "/MANIFESTINPUT:${PROJECT_SOURCE_DIR}/PerMonitorHighDPIAware.manifest")
set_target_properties(WeaselServer PROPERTIES FOLDER "Product")
install(TARGETS WeaselServer RUNTIME DESTINATION .)
