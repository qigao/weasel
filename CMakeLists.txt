cmake_minimum_required(VERSION 3.25)
include(CMakeOptions.cmake)
project(Weasel VERSION 0.17.4 LANGUAGES CXX RC)

if(NOT WIN32)
  message(FATAL_ERROR "Weasel requires a Windows target")
endif()
if(MSVC)
  set(WEASEL_TARGET_ARCH "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
elseif(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(WEASEL_TARGET_ARCH x64)
  elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(WEASEL_TARGET_ARCH ARM64)
  endif()
else()
  message(FATAL_ERROR "Use MSVC or LLVM-MinGW targeting Windows x64/ARM64")
endif()
if(NOT WEASEL_TARGET_ARCH MATCHES "^(x64|ARM64)$")
  message(FATAL_ERROR "Weasel supports Windows x64 and ARM64 CMake targets")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# A missing OpenWTL header must not resolve to Microsoft's ATL/MFC.
if(MSVC)
  file(TO_CMAKE_PATH "$ENV{INCLUDE}" _weasel_sdk_includes)
  list(FILTER _weasel_sdk_includes EXCLUDE REGEX "[Aa][Tt][Ll][Mm][Ff][Cc](/|$)")
  if(NOT _weasel_sdk_includes)
    message(FATAL_ERROR "Initialize VsDevCmd.bat before configuring Weasel")
  endif()
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${_weasel_sdk_includes}")
  set(CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES "${_weasel_sdk_includes}")
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/X>"
                      "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>"
                      "$<$<COMPILE_LANGUAGE:RC>:/x>")
endif()

add_subdirectory(external/OpenWTL EXCLUDE_FROM_ALL)
if(MINGW)
  add_compile_options("$<$<COMPILE_LANGUAGE:RC>:/x>")
  include(cmake/WeaselTsfSdk.cmake)
endif()
find_package(Boost CONFIG REQUIRED COMPONENTS interprocess serialization wserialization thread)

# Product headers expose serialization types to UI consumers until issue #2.
add_library(weasel_headers INTERFACE)
target_include_directories(weasel_headers INTERFACE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(weasel_headers INTERFACE
  OpenWTL::WTL Boost::headers Boost::disable_autolinking)
target_compile_definitions(weasel_headers INTERFACE
  VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  VERSION_MINOR=${PROJECT_VERSION_MINOR}
  VERSION_PATCH=${PROJECT_VERSION_PATCH}
  PRODUCT_VERSION=${PROJECT_VERSION}.0 FILE_VERSION=${PROJECT_VERSION}.0)
set_target_properties(weasel_headers PROPERTIES FOLDER "Support")

add_subdirectory(WeaselIPC)
add_subdirectory(WeaselUI)
add_subdirectory(WeaselTSF)
add_subdirectory(WeaselSetup)
if(WEASEL_BUILD_APPLICATIONS)
  include(cmake/WeaselApplicationDependencies.cmake)
  add_subdirectory(WeaselIPCServer)
  add_subdirectory(RimeWithWeasel)
  add_subdirectory(WeaselServer)
  add_subdirectory(WeaselDeployer)
endif()
if(WEASEL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
