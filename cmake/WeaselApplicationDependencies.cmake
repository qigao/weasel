# The existing acquisition workflow produces one x64 dependency bundle in
# include/, lib64/ and output/. Do not search for another Rime or WinSparkle.
if(NOT CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
  message(FATAL_ERROR "Server/Deployer require the x64 Rime and WinSparkle bundle; ARM64 applications are not qualified")
endif()
if(NOT DEFINED ENV{WEASEL_DEPENDENCY_ROOT} OR
   NOT IS_DIRECTORY "$ENV{WEASEL_DEPENDENCY_ROOT}")
  message(FATAL_ERROR "WEASEL_DEPENDENCY_ROOT must name the existing Weasel dependency bundle")
endif()
foreach(_file IN ITEMS include/rime_api.h include/rime_levers_api.h
    lib64/rime.lib output/rime.dll lib64/WinSparkle.lib output/WinSparkle.dll)
  if(NOT EXISTS "$ENV{WEASEL_DEPENDENCY_ROOT}/${_file}")
    message(FATAL_ERROR "Missing acquired dependency: $ENV{WEASEL_DEPENDENCY_ROOT}/${_file}. See docs/CMAKE_BUILD.md")
  endif()
endforeach()

add_library(Rime::Rime SHARED IMPORTED GLOBAL)
set_target_properties(Rime::Rime PROPERTIES
  IMPORTED_IMPLIB "$ENV{WEASEL_DEPENDENCY_ROOT}/lib64/rime.lib"
  IMPORTED_LOCATION "$ENV{WEASEL_DEPENDENCY_ROOT}/output/rime.dll"
  INTERFACE_INCLUDE_DIRECTORIES "$ENV{WEASEL_DEPENDENCY_ROOT}/include")
add_library(WinSparkle::WinSparkle SHARED IMPORTED GLOBAL)
set_target_properties(WinSparkle::WinSparkle PROPERTIES
  IMPORTED_IMPLIB "$ENV{WEASEL_DEPENDENCY_ROOT}/lib64/WinSparkle.lib"
  IMPORTED_LOCATION "$ENV{WEASEL_DEPENDENCY_ROOT}/output/WinSparkle.dll")
