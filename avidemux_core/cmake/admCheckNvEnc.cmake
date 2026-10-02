MACRO(checkNvEnc)
    IF(NOT NVENC_CHECKED)
        OPTION(NVENC "" ON)

        MESSAGE(STATUS "Checking for NVENC")
        MESSAGE(STATUS "*****************")

        IF(NVENC)
            PKG_CHECK_MODULES(FFNVENC ffnvcodec)
            IF(FFNVENC_FOUND)
                # pkg-config reports the include root (e.g. /ucrt64/include on
                # MSYS2) while nv-codec-headers installs its public headers in
                # the ffnvcodec subdirectory. FFNVENC_CFLAGS contains compiler
                # flags and must not be treated as filesystem paths.
                FIND_PATH(NVENC_INCLUDE_DIR dynlink_loader.h
                        HINTS ${FFNVENC_INCLUDE_DIRS} ${FFNVENC_INCLUDEDIR}
                        PATHS /usr/local/include /usr/include /include ${CROSS}/include
                        PATH_SUFFIXES ffnvcodec)
                IF(NVENC_INCLUDE_DIR)
                    MESSAGE(STATUS "NVENC header found in ${NVENC_INCLUDE_DIR}")
                    SET(USE_NVENC True CACHE BOOL "")
                    SET(NVENC_FOUND 1)
                ELSE()
                    MESSAGE(STATUS "NVENC header not found")
                    SET(NVENC_FOUND 0)
                ENDIF()
            ELSE()
                MESSAGE(STATUS "FFNVENC not found, you can get it from here https://github.com/FFmpeg/nv-codec-headers")
            ENDIF()
            SET(NVENC_CHECKED 1 CACHE INTERNAL "")
        ENDIF()

        MESSAGE("")
	APPEND_SUMMARY_LIST("Video Encoder" "NVENC" "${NVENC_FOUND}")
    ENDIF()
ENDMACRO()
