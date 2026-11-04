cmake_minimum_required(VERSION 3.25)
project(Boomify VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(Boomify WIN32 src/main_v17_nav.cpp src/crash_guard.cpp)
if(NOT DEFINED BOOMIFY_BUILD_SHA)
  set(BOOMIFY_BUILD_SHA "local")
endif()
target_compile_definitions(Boomify PRIVATE UNICODE _UNICODE NOMINMAX BOOMIFY_BUILD_SHA=\"${BOOMIFY_BUILD_SHA}\")
if(MSVC)
  target_compile_options(Boomify PRIVATE /utf-8)
endif()
target_link_libraries(Boomify PRIVATE winmm comdlg32 urlmon advapi32 shell32)

# VST3 support is provided by Steinberg's MIT-licensed SDK. Cache the SDK outside
# the ephemeral build/ directory because build.bat creates a clean build each time.
option(BOOMIFY_ENABLE_VST3 "Build out-of-process VST3 instrument renderer" ON)
if(BOOMIFY_ENABLE_VST3)
  include(FetchContent)
  set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.boomify_dependencies" CACHE PATH "Boomify dependency cache")
  set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
  set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG v3.8.1_build_84
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    GIT_SUBMODULES_RECURSE TRUE)
  FetchContent_MakeAvailable(vst3sdk)
  add_executable(BoomifyVST3Host
    src/vst3_host.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module_win32.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/plugprovider.cpp
  )
  target_compile_definitions(BoomifyVST3Host PRIVATE UNICODE _UNICODE NOMINMAX)
  if(MSVC)
    target_compile_options(BoomifyVST3Host PRIVATE /utf-8)
  endif()
  target_link_libraries(BoomifyVST3Host PRIVATE sdk_hosting sdk_common pluginterfaces base)
  # Keep both Boomify.exe and the host in build/Release for the automatic build.
  set_target_properties(BoomifyVST3Host PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()
