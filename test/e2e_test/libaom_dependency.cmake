include(ExternalProject)
#/property:Configuration=${CMAKE_BUILD_TYPE} ${CMAKE_BINARY_DIR}/libaom/src/DepLibAom-build/
if (MSVC OR MSYS OR MINGW OR WIN32)
set(TARGET_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/aom/lib/msvc/Release/")
set(TARGET "MinSizeRel/aom.lib")
set(CUSTOM_CONFIG -G "Visual Studio 15 2017 Win64")
set(CUSTOM_BUILD_CMD msbuild /p:Configuration=MinSizeRel /t:Rebuild AOM.sln)
endif(MSVC OR MSYS OR MINGW OR WIN32)

if (UNIX AND NOT APPLE)
set(TARGET_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/aom/lib/linux")
set(TARGET "libaom.so")
set(CUSTOM_CONFIG "-DBUILD_SHARED_LIBS=1")
set(CUSTOM_BUILD_CMD make aom)
endif(UNIX AND NOT APPLE)

if (APPLE)
set(TARGET_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/aom/lib/mac")
set(TARGET "libaom*dylib")
set(CUSTOM_CONFIG "-DBUILD_SHARED_LIBS=1")
set(CUSTOM_BUILD_CMD make aom)
endif(APPLE)


ExternalProject_Add(DepLibAom
  PREFIX "${CMAKE_BINARY_DIR}/libaom"
  GIT_REPOSITORY "https://github.com/Cidana-Developers/aom.git"
  GIT_TAG "av1-normative"
  CMAKE_ARGS
    ${CUSTOM_CONFIG}
    -DCONFIG_INSPECTION=1
	-DENABLE_TESTS=0
  BUILD_COMMAND ${CUSTOM_BUILD_CMD}
  BUILD_ALWAYS 0
  INSTALL_COMMAND ""
  )

add_custom_command(TARGET DepLibAom POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_BINARY_DIR}/libaom/src/DepLibAom-build/${TARGET} ${TARGET_OUTPUT_PATH})