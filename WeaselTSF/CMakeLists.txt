add_library(WeaselTSF SHARED
  CandidateList.cpp Compartment.cpp Composition.cpp DisplayAttribute.cpp
  DisplayAttributeInfo.cpp DisplayAttributeProvider.cpp dllmain.cpp
  EditSession.cpp EnumDisplayAttributeInfo.cpp Globals.cpp KeyEvent.cpp
  KeyEventSink.cpp LanguageBar.cpp Register.cpp Server.cpp
  TextEditSink.cpp ThreadMgrEventSink.cpp WeaselTSF.cpp
  WeaselTSF.rc WeaselTSF.def)
target_link_libraries(WeaselTSF PRIVATE WeaselIPC WeaselUI)
target_compile_definitions(WeaselTSF PRIVATE WEASELTSF_EXPORTS)
target_include_directories(WeaselTSF PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
  set(_weasel_dll_name weaselARM64)
  target_compile_definitions(WeaselTSF PRIVATE "$<$<COMPILE_LANGUAGE:RC>:ARM64>")
else()
  set(_weasel_dll_name weaselx64)
  target_compile_definitions(WeaselTSF PRIVATE "$<$<COMPILE_LANGUAGE:RC>:WIN64>")
endif()
set_target_properties(WeaselTSF PROPERTIES OUTPUT_NAME "${_weasel_dll_name}" FOLDER "Product")
install(TARGETS WeaselTSF RUNTIME DESTINATION .)
