# MinGW's TSF declarations are incomplete. Use the selected native Windows
# SDK's generated ABI headers and UUID library, alongside MinGW's Win32/CRT.
# These SDK files stay in the build tree and are never redistributed.
if(NOT CMAKE_RC_COMPILER MATCHES "/rc\\.exe$" OR
   NOT CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES)
  message(FATAL_ERROR "LLVM TSF builds require WEASEL_WINDOWS_SDK_BIN with native rc.exe")
endif()
list(GET CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES 0 _weasel_sdk_um)
set(_weasel_tsf_headers "${CMAKE_BINARY_DIR}/tsf-sdk")
file(MAKE_DIRECTORY "${_weasel_tsf_headers}")
include("${PROJECT_SOURCE_DIR}/external/OpenWTL/cmake/OpenATLRibbon.cmake")
foreach(_header IN ITEMS msctf.h ctfutb.h textstor.h ctffunc.h)
  configure_file("${_weasel_sdk_um}/${_header}"
    "${_weasel_tsf_headers}/${_header}" COPYONLY)
  # Bind COM interfaces only. SDK UUID-decorated typedefs are aliases of
  # primitive types and cannot have independent MinGW UUID specializations.
  file(READ "${_weasel_tsf_headers}/${_header}" _weasel_sdk_text)
  string(REGEX MATCHALL "MIDL_INTERFACE\\(\"[0-9a-fA-F-]+\"\\)[ \t\r\n]+[A-Za-z_0-9]+[ \t\r\n]*:[ \t\r\n]*public"
    _weasel_sdk_interfaces "${_weasel_sdk_text}")
  set(uuids "")
  set(definitions "")
  foreach(_interface IN LISTS _weasel_sdk_interfaces)
    string(REGEX MATCH "MIDL_INTERFACE\\(\"([0-9a-fA-F-]+)\"\\)[ \t\r\n]+([A-Za-z_0-9]+)"
      _match "${_interface}")
    _openatl_ribbon_guid("${CMAKE_MATCH_2}" "${CMAKE_MATCH_1}" "unused")
  endforeach()
  string(MAKE_C_IDENTIFIER "WEASEL_TSF_SDK_UUID_${_header}" _weasel_uuid_guard)
  file(APPEND "${_weasel_tsf_headers}/${_header}"
    "\n#if defined(__cplusplus) && defined(__MINGW32__) && !defined(${_weasel_uuid_guard})\n#define ${_weasel_uuid_guard}\n${uuids}#endif\n")
endforeach()
get_filename_component(_weasel_sdk_version_dir "${_weasel_sdk_um}" DIRECTORY)
get_filename_component(_weasel_sdk_version "${_weasel_sdk_version_dir}" NAME)
get_filename_component(_weasel_sdk_root "${_weasel_sdk_version_dir}/../.." ABSOLUTE)
string(TOLOWER "${WEASEL_TARGET_ARCH}" _weasel_sdk_arch)
set(_weasel_sdk_uuid "${_weasel_sdk_root}/Lib/${_weasel_sdk_version}/um/${_weasel_sdk_arch}/uuid.lib")
if(NOT EXISTS "${_weasel_sdk_uuid}")
  message(FATAL_ERROR "Missing selected Windows SDK UUID library: ${_weasel_sdk_uuid}")
endif()
add_library(weasel_tsf_sdk INTERFACE)
target_include_directories(weasel_tsf_sdk SYSTEM INTERFACE "${_weasel_tsf_headers}")
add_library(weasel_sdk_uuid STATIC IMPORTED)
set_target_properties(weasel_sdk_uuid PROPERTIES IMPORTED_LOCATION "${_weasel_sdk_uuid}")
set_target_properties(weasel_tsf_sdk PROPERTIES FOLDER "Support")
