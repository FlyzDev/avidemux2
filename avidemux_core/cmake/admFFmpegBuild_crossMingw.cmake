INCLUDE(admFFmpegBuild_helpers)

#@@
ADM_FF_SET_DEFAULT()

IF(USE_NVENC)
  SET(FFMPEG_ENCODERS ${FFMPEG_ENCODERS} h264_nvenc hevc_nvenc av1_nvenc)
  SET(FFMPEG_DECODERS ${FFMPEG_DECODERS} nvdec)
ENDIF()



#@@
ADM_FF_PATCH_IF_NEEDED()

IF("${CROSS_C_COMPILER}" STREQUAL "clang")
  patch_file("${FFMPEG_SOURCE_DIR}" "${FFMPEG_PATCH_DIR}/clang_win32_workaround.diff")
ENDIF()

IF(ADM_CPU_X86_32)
  IF("${CROSS_C_COMPILER}" STREQUAL "clang")
    # With clang we use the -mstackrealign -mstack-alignment=16
  ELSE()
    # Old win32, for hack, not sure it really works with recent gcc
    # Not anymore xadd(--enable-memalign-hack)
  ENDIF()
ENDIF()

xadd(--enable-w32threads)

#  Cross compiler override (win32 & win64)
xadd(--host-cc gcc)
IF(CMAKE_HOST_WIN32)
  # Native MSYS2/UCRT64 exposes binutils as ar/nm on PATH.
  xadd(--nm nm)
ELSE()
  xadd(--nm ${CMAKE_CROSS_PREFIX}-nm)
ENDIF()
IF(CMAKE_HOST_WIN32 AND MINGW AND NOT CROSS)
  GET_FILENAME_COMPONENT(ADM_NATIVE_MINGW_BIN "${CMAKE_C_COMPILER}" DIRECTORY)
  GET_FILENAME_COMPONENT(ADM_NATIVE_MINGW_ROOT "${ADM_NATIVE_MINGW_BIN}" DIRECTORY)
  MESSAGE(STATUS "Native MinGW root for FFmpeg: ${ADM_NATIVE_MINGW_ROOT}")
  xadd(--extra-cflags -I${ADM_NATIVE_MINGW_ROOT}/include)
ELSE()
  xadd(--extra-cflags -I${CROSS}/include)
ENDIF()
IF(CMAKE_C_FLAGS)
  xadd(--extra-cflags ${CMAKE_C_FLAGS})
  xadd(--extra-ldflags ${CMAKE_C_FLAGS})
ENDIF()

SET(CROSS_OS mingw32)

IF(ADM_CPU_64BIT)
  SET(CROSS_ARCH x86_64)
ELSE()
  SET(CROSS_ARCH i386)
ENDIF()

MESSAGE(STATUS "Using cross compilation flag: ${FFMPEG_FLAGS}")

ADM_FF_ADD_OPTIONS()


xadd(--cc "${CMAKE_C_COMPILER}")
xadd(--ld "${CMAKE_C_COMPILER}")
# Avoid CMAKE_AR's Windows D:/... spelling in FFmpeg's POSIX shell helpers.
# Native MSYS2 uses plain ar, while Linux-hosted MXE uses the prefixed tool.
IF(CMAKE_HOST_WIN32)
  xadd(--ar ar)
ELSE()
  xadd(--ar "${CMAKE_CROSS_PREFIX}-ar")
ENDIF()

ADM_FF_SET_EXTRA_FLAGS()
IF(CROSS)
  xadd(--enable-cross-compile)
ENDIF()

SET(CROSS_ARCH "${CROSS_ARCH}" CACHE STRING "")
xadd(--arch ${CROSS_ARCH})

SET(CROSS_OS "${CROSS_OS}" CACHE STRING "")
xadd(--target-os ${CROSS_OS})

ADM_FF_SET_EXTRA_FLAGS()

IF(USE_DXVA2)
  xadd(--enable-dxva2)
  # We assume 32 bits mean windows XP; disable d3d11
  IF(NOT ADM_CPU_X86_64)
    xadd(--disable-d3d11va)
  ENDIF()
ENDIF()

#@@
ADM_FF_BUILD_UNIX_STYLE()
ADM_FF_ADD_DUMMY_TARGET()
MACRO(FF_ADD_SUBLIB lib)
  ADD_CUSTOM_COMMAND(
    OUTPUT       "${lib}"
    DEPENDS   libavutil_dummy
    COMMAND ${BASH_EXECUTABLE} -c echo "placeHolder")
ENDMACRO()

FF_ADD_SUBLIB("${FFMPEG_BINARY_DIR}/libavutil/${LIBAVUTIL_LIB}")
FF_ADD_SUBLIB("${FFMPEG_BINARY_DIR}/libavcodec/${LIBAVCODEC_LIB}")
FF_ADD_SUBLIB("${FFMPEG_BINARY_DIR}/libavformat/${LIBAVFORMAT_LIB}")
FF_ADD_SUBLIB("${FFMPEG_BINARY_DIR}/libswscale/${LIBSWSCALE_LIB}")
IF(USE_LIBPOSTPROC)
  FF_ADD_SUBLIB("${FFMPEG_BINARY_DIR}/libpostproc/${LIBPOSTPROC_LIB}")
ENDIF()

MACRO(FF_ADD_SUBLIB lib)
  ADD_CUSTOM_COMMAND(
    OUTPUT  "${lib}"
    DEPENDS "${FFMPEG_BINARY_DIR}/libavcodec/${LIBAVCODEC_LIB}"
    COMMAND ${BASH_EXECUTABLE} -c echo "placeHolder")
ENDMACRO()

FF_ADD_SUBLIB("${FFMPEG_BINARY_DIR}/libavutil/${LIBAVUTIL_LIB}")
FF_ADD_SUBLIB("${FFMPEG_BINARY_DIR}/libavformat/${LIBAVFORMAT_LIB}")
FF_ADD_SUBLIB("${FFMPEG_BINARY_DIR}/libswscale/${LIBSWSCALE_LIB}")
IF(USE_LIBPOSTPROC)
  FF_ADD_SUBLIB("${FFMPEG_BINARY_DIR}/libpostproc/${LIBPOSTPROC_LIB}")
ENDIF()

ADM_FF_INSTALL_LIBS_AND_HEADERS()

IF(USE_DXVA2)
  INSTALL(FILES "${FFMPEG_SOURCE_DIR}/libavcodec/dxva2.h" DESTINATION "${AVIDEMUX_INSTALL_INCLUDE_DIR}/avidemux/${AVIDEMUX_MAJOR_MINOR}/libavcodec" COMPONENT dev)
  INSTALL(FILES "${FFMPEG_SOURCE_DIR}/libavcodec/dxva2_internal.h" DESTINATION "${AVIDEMUX_INSTALL_INCLUDE_DIR}/avidemux/${AVIDEMUX_MAJOR_MINOR}/libavcodec" COMPONENT dev)
ENDIF()

#
#
#
