add_executable(WeaselSetup WIN32
  imesetup.cpp InstallOptionsDlg.cpp WeaselSetup.cpp WeaselSetup.rc)
target_link_libraries(WeaselSetup PRIVATE weasel_headers shcore)
target_include_directories(WeaselSetup PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
if(MSVC)
  target_link_options(WeaselSetup PRIVATE /MANIFEST:EMBED
    "/MANIFESTINPUT:${PROJECT_SOURCE_DIR}/PerMonitorHighDPIAware.manifest")
else()
  target_sources(WeaselSetup PRIVATE WeaselSetup.manifest.rc)
endif()
set_target_properties(WeaselSetup PROPERTIES FOLDER "Product")
install(TARGETS WeaselSetup RUNTIME DESTINATION .)
