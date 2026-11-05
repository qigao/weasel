add_executable(WeaselDeployer
  Configurator.cpp DictManagementDialog.cpp FontSettingDialog.cpp
  SwitcherSettingsDialog.cpp UIStyleSettings.cpp UIStyleSettingsDialog.cpp
  WeaselDeployer.cpp WeaselDeployer.rc)
target_link_libraries(WeaselDeployer PRIVATE RimeWithWeasel WeaselIPC)
target_include_directories(WeaselDeployer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
if(MSVC)
  target_link_options(WeaselDeployer PRIVATE /MANIFEST:EMBED
    "/MANIFESTINPUT:${PROJECT_SOURCE_DIR}/PerMonitorHighDPIAware.manifest")
else()
  target_sources(WeaselDeployer PRIVATE WeaselDeployer.manifest.rc)
  target_link_options(WeaselDeployer PRIVATE -municode)
endif()
set_target_properties(WeaselDeployer PROPERTIES FOLDER "Product")
install(TARGETS WeaselDeployer RUNTIME DESTINATION .)
