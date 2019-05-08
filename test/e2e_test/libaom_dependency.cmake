include(ExternalProject)

if (MSVC OR MSYS OR MINGW OR WIN32)
set(TARGET_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/aom/lib/msvc/${CMAKE_BUILD_TYPE}")
set(TARGET "aom.lib")
set(CUSTOM_CONFIG "")
endif(MSVC OR MSYS OR MINGW OR WIN32)

if (UNIX AND NOT APPLE)
set(TARGET_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/aom/lib/linux")
set(TARGET "libaom.so")
set(CUSTOM_CONFIG "-DBUILD_SHARED_LIBS=1")
endif(UNIX AND NOT APPLE)

if (APPLE)
set(TARGET_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/aom/lib/mac")
set(TARGET "libaom*dylib")
set(CUSTOM_CONFIG "-DBUILD_SHARED_LIBS=1")
endif(APPLE)


ExternalProject_Add(DepLibAom
  PREFIX "${CMAKE_BINARY_DIR}/libaom"
  GIT_REPOSITORY "https://github.com/Cidana-Developers/aom.git"
  GIT_TAG "av1-normative"
  CMAKE_ARGS
    ${CUSTOM_CONFIG}
    -DCONFIG_INSPECTION=1
  BUILD_COMMAND make aom
  BUILD_ALWAYS 0
  INSTALL_COMMAND ""
  )

add_custom_command(TARGET DepLibAom POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_BINARY_DIR}/libaom/src/DepLibAom-build/${TARGET} ${TARGET_OUTPUT_PATH})