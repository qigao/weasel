add_library(WeaselTSF SHARED
  CandidateList.cpp Compartment.cpp Composition.cpp DisplayAttribute.cpp
  DisplayAttributeInfo.cpp DisplayAttributeProvider.cpp dllmain.cpp
  EditSession.cpp EnumDisplayAttributeInfo.cpp Globals.cpp KeyEvent.cpp
  KeyEventSink.cpp LanguageBar.cpp Register.cpp Server.cpp
  TextEditSink.cpp ThreadMgrEventSink.cpp WeaselTSF.cpp
  WeaselTSF.rc WeaselTSF.def)
if(MINGW)
  # Resolve TSF GUIDs from the complete SDK before MinGW's partial uuid archive.
  target_link_libraries(WeaselTSF PRIVATE weasel_sdk_uuid weasel_tsf_sdk)
endif()
target_link_libraries(WeaselTSF PRIVATE WeaselIPC WeaselUI)
target_compile_definitions(WeaselTSF PRIVATE WEASELTSF_EXPORTS)
target_include_directories(WeaselTSF PRIVATE
  "$<$<COMPILE_LANGUAGE:RC>:${CMAKE_CURRENT_SOURCE_DIR}>")
if(WEASEL_TARGET_ARCH STREQUAL "ARM64")
  set(_weasel_dll_name weaselARM64)
  target_compile_definitions(WeaselTSF PRIVATE "$<$<COMPILE_LANGUAGE:RC>:ARM64>")
else()
  set(_weasel_dll_name weaselx64)
  target_compile_definitions(WeaselTSF PRIVATE "$<$<COMPILE_LANGUAGE:RC>:WIN64>")
endif()
set_target_properties(WeaselTSF PROPERTIES PREFIX "" OUTPUT_NAME "${_weasel_dll_name}" FOLDER "Product")
install(TARGETS WeaselTSF RUNTIME DESTINATION .)
