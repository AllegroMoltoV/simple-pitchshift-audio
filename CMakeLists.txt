cmake_minimum_required(VERSION 3.21)

project(simple-pitchshift-audio VERSION 1.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Link the C runtime statically so users need no Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
  signalsmith-linear
  SYSTEM
  GIT_REPOSITORY https://github.com/Signalsmith-Audio/linear.git
  GIT_TAG 7bb1c8f36794d6980080ce817c91f8da8fe56a66
  GIT_SUBMODULES ""
)
FetchContent_Declare(
  signalsmith-stretch
  SYSTEM
  GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
  GIT_TAG 88061fd4eda56a2353c5d65c74aa3f4ccf76f9da
  GIT_SUBMODULES ""
)
# clap-wrapper v0.17.0. It downloads the CLAP and VST 3 SDKs at the versions it pins.
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "" FORCE)
set(CLAP_WRAPPER_BUILD_AUV2 FALSE CACHE BOOL "" FORCE)
FetchContent_Declare(
  clap-wrapper
  GIT_REPOSITORY https://github.com/free-audio/clap-wrapper.git
  GIT_TAG 806bba0a5f792e3966b99c1561ab9952d3316d7c
)
FetchContent_MakeAvailable(signalsmith-linear signalsmith-stretch clap-wrapper)

set(PRODUCT_NAME "Simple Pitch Shift")

add_library(simple-pitchshift-impl STATIC src/plugin.cpp)
target_include_directories(simple-pitchshift-impl PUBLIC src)
# clapwrapper/vst3.h, for the VST 3 factory information returned in src/plugin.cpp.
target_include_directories(simple-pitchshift-impl PRIVATE "${clap-wrapper_SOURCE_DIR}/include")
target_compile_definitions(simple-pitchshift-impl PRIVATE SIMPLE_PITCHSHIFT_VERSION="${PROJECT_VERSION}")
target_link_libraries(simple-pitchshift-impl PUBLIC clap signalsmith-stretch)

make_clapfirst_plugins(
  TARGET_NAME simple-pitchshift
  IMPL_TARGET simple-pitchshift-impl
  OUTPUT_NAME "${PRODUCT_NAME}"
  ENTRY_SOURCE src/plugin-entry.cpp
  BUNDLE_IDENTIFIER "jp.allegromoltov.simple-pitchshift"
  BUNDLE_VERSION ${PROJECT_VERSION}
  COPY_AFTER_BUILD FALSE
  # The VST 3 validator and the registry both expect the bundle folder layout on Windows.
  WINDOWS_FOLDER_VST3 TRUE
  PLUGIN_FORMATS CLAP VST3
  ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/assets
)

# Release layout: `cmake --install build --config Release --prefix <dir>`, then zip <dir>.
# The archive carries the license texts of everything linked into the plugins.
# clap-wrapper downloads the SDKs to these places; a layout change in a new clap-wrapper fails here.
set(VST3_SDK_LICENSE "${CMAKE_BINARY_DIR}/cpm/vst3sdk/LICENSE.txt")
set(CLAP_SDK_LICENSE "${clap-wrapper_BINARY_DIR}/cpm/clap/LICENSE")
foreach(license IN ITEMS "${VST3_SDK_LICENSE}" "${CLAP_SDK_LICENSE}")
  if(NOT EXISTS "${license}")
    message(FATAL_ERROR "License file not found: ${license}")
  endif()
endforeach()
# fmt keeps its license in the leading comment of format.h.
file(READ "${clap-wrapper_SOURCE_DIR}/libs/fmt/fmt/format.h" FMT_HEADER)
string(FIND "${FMT_HEADER}" "*/" FMT_COMMENT_END)
string(SUBSTRING "${FMT_HEADER}" 0 ${FMT_COMMENT_END} FMT_LICENSE_TEXT)
string(REGEX REPLACE "^/\\*\n" "" FMT_LICENSE_TEXT "${FMT_LICENSE_TEXT}")
file(WRITE "${CMAKE_BINARY_DIR}/fmt-LICENSE.txt" "${FMT_LICENSE_TEXT}")

install(DIRECTORY "${CMAKE_BINARY_DIR}/assets/VST3/${PRODUCT_NAME}.vst3" DESTINATION VST3)
install(FILES "${CMAKE_BINARY_DIR}/assets/CLAP/$<CONFIG>/${PRODUCT_NAME}.clap" DESTINATION CLAP)
install(FILES LICENSE DESTINATION . RENAME LICENSE.txt)
install(FILES packaging/README.txt DESTINATION .)
install(FILES "${VST3_SDK_LICENSE}" DESTINATION THIRD_PARTY_LICENSES RENAME vst3sdk.txt)
install(FILES "${CLAP_SDK_LICENSE}" DESTINATION THIRD_PARTY_LICENSES RENAME clap.txt)
install(FILES "${clap-wrapper_SOURCE_DIR}/LICENSE" DESTINATION THIRD_PARTY_LICENSES RENAME clap-wrapper.txt)
install(FILES "${CMAKE_BINARY_DIR}/fmt-LICENSE.txt" DESTINATION THIRD_PARTY_LICENSES RENAME fmt.txt)
install(FILES "${signalsmith-stretch_SOURCE_DIR}/LICENSE.txt" DESTINATION THIRD_PARTY_LICENSES
        RENAME signalsmith-stretch.txt)
install(FILES "${signalsmith-linear_SOURCE_DIR}/LICENSE.txt" DESTINATION THIRD_PARTY_LICENSES
        RENAME signalsmith-linear.txt)

include(CTest)
if(BUILD_TESTING)
  add_executable(test-shifter tests/test-shifter.cpp)
  target_include_directories(test-shifter PRIVATE src)
  target_link_libraries(test-shifter PRIVATE signalsmith-stretch)
  add_test(NAME shifter COMMAND test-shifter)
endif()
