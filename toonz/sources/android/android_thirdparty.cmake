#=============================================================================
#
#  Third party libraries for the Android build.
#
#  The desktop build links against libraries that are either prebuilt in
#  thirdparty/ (Windows) or provided by the system (Linux).  Neither applies
#  to Android: there is no system copy of them and the prebuilt binaries are
#  for the wrong architecture, so the sources bundled in thirdparty/ are
#  compiled here, statically, once.
#
#  Building them from the bundled sources also keeps the Android port
#  reproducible: the exact same zlib, libpng, libtiff, libjpeg-turbo, lz4 and
#  minilzo revisions the desktop build uses end up in the APK, so the file
#  formats behave identically.
#
#=============================================================================

get_filename_component(OT_THIRDPARTY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../thirdparty ABSOLUTE)

message(STATUS "Android: building bundled third party libraries from ${OT_THIRDPARTY_ROOT}")

set(OT_TP_INCLUDE_DIRS "")

#-----------------------------------------------------------------------------
# zlib
#-----------------------------------------------------------------------------

set(ZLIB_SOURCES
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/adler32.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/compress.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/crc32.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/deflate.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/gzclose.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/gzlib.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/gzread.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/gzwrite.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/infback.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/inffast.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/inflate.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/inftrees.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/trees.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/uncompr.c
    ${OT_THIRDPARTY_ROOT}/zlib-1.2.8/zutil.c
)

add_library(ot_zlib STATIC ${ZLIB_SOURCES})
target_include_directories(ot_zlib PUBLIC ${OT_THIRDPARTY_ROOT}/zlib-1.2.8)
target_compile_definitions(ot_zlib PRIVATE
    -DHAVE_HIDDEN
    -DZ_HAVE_UNISTD_H
)
set(Z_LIB ot_zlib)
list(APPEND OT_TP_INCLUDE_DIRS ${OT_THIRDPARTY_ROOT}/zlib-1.2.8)

#-----------------------------------------------------------------------------
# libpng
#-----------------------------------------------------------------------------

# libpng requires a generated pnglibconf.h; the prebuilt one shipped with the
# sources is what the desktop build uses as well.
if(NOT EXISTS ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pnglibconf.h)
    configure_file(
        ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/scripts/pnglibconf.h.prebuilt
        ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pnglibconf.h
        COPYONLY
    )
endif()

set(PNG_SOURCES
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/png.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngerror.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngget.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngmem.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngpread.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngread.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngrio.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngrtran.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngrutil.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngset.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngtrans.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngwio.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngwrite.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngwtran.c
    ${OT_THIRDPARTY_ROOT}/libpng-1.6.21/pngwutil.c
)

add_library(ot_png STATIC ${PNG_SOURCES})
target_include_directories(ot_png PUBLIC ${OT_THIRDPARTY_ROOT}/libpng-1.6.21)
target_compile_definitions(ot_png PRIVATE -DPNG_NO_READ_tIME)
target_link_libraries(ot_png PUBLIC ot_zlib)
set(PNG_LIB ot_png)
list(APPEND OT_TP_INCLUDE_DIRS ${OT_THIRDPARTY_ROOT}/libpng-1.6.21)

#-----------------------------------------------------------------------------
# libjpeg-turbo
#
# Only the codec is needed: OpenToonz uses it for the JPEG levels and for the
# JPEG compression inside TIFF documents.  The SIMD variants are left out - the
# scalar paths produce bit identical output and keep the build portable across
# the ABIs.
#-----------------------------------------------------------------------------

set(JPEG_SOURCES
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jaricom.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcapimin.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcapistd.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcarith.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jccoefct.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jccolor.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcdctmgr.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jchuff.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcicc.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcinit.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcmainct.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcmarker.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcmaster.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcomapi.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcparam.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcphuff.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcprepct.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jcsample.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jctrans.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdapimin.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdapistd.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdarith.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdatadst.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdatasrc.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdcoefct.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdcolor.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jddctmgr.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdhuff.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdicc.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdinput.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdmainct.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdmarker.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdmaster.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdmerge.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdphuff.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdpostct.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdsample.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jdtrans.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jerror.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jfdctflt.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jfdctfst.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jfdctint.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jidctflt.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jidctfst.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jidctint.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jidctred.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jmemmgr.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jmemnobs.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jquant1.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jquant2.c
    ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6/jutils.c
)

# jconfig.h / jconfigint.h are normally generated by the upstream CMake build;
# the minimal configuration below is what an NDK build needs.
set(OT_JPEG_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/jpeg-config)
file(MAKE_DIRECTORY ${OT_JPEG_GENERATED_DIR})

file(WRITE ${OT_JPEG_GENERATED_DIR}/jconfig.h
"/* Generated for the Android build - see android_thirdparty.cmake. */
#define JPEG_LIB_VERSION 62
#define LIBJPEG_TURBO_VERSION 2.0.6
#define LIBJPEG_TURBO_VERSION_NUMBER 2000006
#define C_ARITH_CODING_SUPPORTED 1
#define D_ARITH_CODING_SUPPORTED 1
#define MEM_SRCDST_SUPPORTED 1
#define BITS_IN_JSAMPLE 8
#define HAVE_LOCALE_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDLIB_H 1
#define HAVE_UNSIGNED_CHAR 1
#define HAVE_UNSIGNED_SHORT 1
")

file(WRITE ${OT_JPEG_GENERATED_DIR}/jconfigint.h
"/* Generated for the Android build - see android_thirdparty.cmake. */
#define BUILD \"20240101\"
#undef inline
#if defined(__GNUC__)
#define INLINE inline __attribute__((always_inline))
#else
#define INLINE inline
#endif
#define THREAD_LOCAL __thread
#define PACKAGE_NAME \"libjpeg-turbo\"
#define VERSION \"2.0.6\"
#define SIZEOF_SIZE_T 8
#define HAVE_BUILTIN_CTZL
")

add_library(ot_jpeg STATIC ${JPEG_SOURCES})
target_include_directories(ot_jpeg PUBLIC ${OT_THIRDPARTY_ROOT}/libjpeg-turbo/libjpeg-turbo-2.0.6)
target_include_directories(ot_jpeg PRIVATE ${OT_JPEG_GENERATED_DIR})
target_compile_definitions(ot_jpeg PRIVATE -DNO_GETENV)
set(JPEG_LIB ot_jpeg)

#-----------------------------------------------------------------------------
# libtiff
#-----------------------------------------------------------------------------

# libtiff needs two generated configuration headers.
set(OT_TIFF_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/tiff-config)
file(MAKE_DIRECTORY ${OT_TIFF_GENERATED_DIR})

file(WRITE ${OT_TIFF_GENERATED_DIR}/tif_config.h
"/* Generated for the Android build - see android_thirdparty.cmake. */
#define HAVE_ASSERT_H 1
#define HAVE_FCNTL_H 1
#define HAVE_FLOOR 1
#define HAVE_INTTYPES_H 1
#define HAVE_ISASCII 1
#define HAVE_LIMITS_H 1
#define HAVE_MEMORY_H 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMSET 1
#define HAVE_MMAP 1
#define HAVE_POW 1
#define HAVE_PTHREAD 1
#define HAVE_SEARCH_H 1
#define HAVE_SETMODE 1
#define HAVE_SQRT 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRCASECMP 1
#define HAVE_STRCHR 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE_STRTOULL 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_IEEEFP 1
#define HOST_FILLORDER FILLORDER_LSB2MSB
#define HOST_BIGENDIAN 0
#define CCITT_SUPPORT 1
#define JPEG_SUPPORT 1
#define LOGLUV_SUPPORT 1
#define LZW_SUPPORT 1
#define NEXT_SUPPORT 1
#define PACKBITS_SUPPORT 1
#define PIXARLOG_SUPPORT 1
#define THUNDER_SUPPORT 1
#define ZIP_SUPPORT 1
#define STRIPCHOP_DEFAULT TIFF_STRIPCHOP
#define SUBIFD_SUPPORT 1
#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1
#define MDI_SUPPORT 1
#define STRIP_SIZE_DEFAULT 8192
#define TIFF_MAX_DIR_COUNT 1048576
")

file(WRITE ${OT_TIFF_GENERATED_DIR}/tiffconf.h
"/* Generated for the Android build - see android_thirdparty.cmake. */
#ifndef _TIFFCONF_
#define _TIFFCONF_
#define CCITT_SUPPORT 1
#define JPEG_SUPPORT 1
#define LOGLUV_SUPPORT 1
#define LZW_SUPPORT 1
#define NEXT_SUPPORT 1
#define PACKBITS_SUPPORT 1
#define PIXARLOG_SUPPORT 1
#define THUNDER_SUPPORT 1
#define ZIP_SUPPORT 1
#define STRIPCHOP_DEFAULT TIFF_STRIPCHOP
#define SUBIFD_SUPPORT 1
#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1
#define COLORIMETRY_SUPPORT 1
#define YCBCR_SUPPORT 1
#define CMYK_SUPPORT 1
#define ICC_SUPPORT 1
#define PHOTOSHOP_SUPPORT 1
#define IPTC_SUPPORT 1
#endif
")

set(TIFF_SOURCES
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_aux.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_close.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_codec.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_color.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_compress.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_dir.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_dirinfo.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_dirread.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_dirwrite.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_dumpmode.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_error.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_extension.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_fax3.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_fax3sm.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_flush.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_getimage.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_jbig.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_jpeg.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_luv.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_lzma.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_lzw.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_next.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_ojpeg.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_open.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_packbits.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_pixarlog.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_predict.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_print.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_read.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_strip.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_swab.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_thunder.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_tile.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_unix.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_version.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_warning.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_write.c
    ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff/tif_zip.c
)

add_library(ot_tiff STATIC ${TIFF_SOURCES})
target_include_directories(ot_tiff
    PUBLIC ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff
    PRIVATE ${OT_TIFF_GENERATED_DIR}
)
target_compile_definitions(ot_tiff PRIVATE -DTIF_PLATFORM_UNIX=1)
target_link_libraries(ot_tiff PUBLIC ot_jpeg ot_zlib)
set(TIFF_LIB ot_tiff)
list(APPEND OT_TP_INCLUDE_DIRS ${OT_THIRDPARTY_ROOT}/tiff-4.0.3/libtiff)

#-----------------------------------------------------------------------------
# LZ4
#-----------------------------------------------------------------------------

add_library(ot_lz4 STATIC
    ${OT_THIRDPARTY_ROOT}/Lz4/Lz4_131/lib/lz4.c
    ${OT_THIRDPARTY_ROOT}/Lz4/Lz4_131/lib/lz4hc.c
    ${OT_THIRDPARTY_ROOT}/Lz4/Lz4_131/lib/lz4frame.c
    ${OT_THIRDPARTY_ROOT}/Lz4/Lz4_131/lib/xxhash.c
)
target_include_directories(ot_lz4 PUBLIC ${OT_THIRDPARTY_ROOT}/Lz4/Lz4_131/lib)
target_compile_definitions(ot_lz4 PRIVATE -DLZ4_STATIC=1)
set(LZ4_LIB ot_lz4)
list(APPEND OT_TP_INCLUDE_DIRS ${OT_THIRDPARTY_ROOT}/Lz4/Lz4_131/lib)

#-----------------------------------------------------------------------------
# minilzo
#
# The LZO raster codec is normally driven through the lzocompress /
# lzodecompress helper executables.  An APK cannot spawn helper binaries, so
# the same algorithm - the one the helpers use, lzo1x_1 - is linked into the
# application directly.  The on-disk format is unchanged.
#-----------------------------------------------------------------------------

add_library(ot_lzo STATIC ${OT_THIRDPARTY_ROOT}/lzo/2.03/minilzo/minilzo.c)
target_include_directories(ot_lzo
    PUBLIC
        ${OT_THIRDPARTY_ROOT}/lzo/2.03/include/lzo
        ${OT_THIRDPARTY_ROOT}/lzo/2.03/minilzo
)
target_compile_definitions(ot_lzo PRIVATE -DLZO_CFG_USE_INTERNAL_LZODEFS=0)
set(LZO_LIB ot_lzo)
# tcodec.cpp includes the codec directly on Android (see the comment above), so
# the headers have to be on the global include path as well.
list(APPEND OT_TP_INCLUDE_DIRS
    ${OT_THIRDPARTY_ROOT}/lzo/2.03/include/lzo
    ${OT_THIRDPARTY_ROOT}/lzo/2.03/minilzo
)

#-----------------------------------------------------------------------------
# SuperLU + OpenBLAS (plastic deformer linear solver)
#
# The third party directory ships SuperLU as a Windows import library only, so
# the sources are compiled here.  OpenBLAS is replaced by the reference BLAS
# that ships with SuperLU (thirdparty/superlu/SuperLU_4.1/CBLAS), which removes
# an architecture specific dependency without changing the solver.
#-----------------------------------------------------------------------------

file(GLOB OT_SUPERLU_SOURCES ${OT_THIRDPARTY_ROOT}/superlu/SuperLU_4.1/SRC/*.c)
list(FILTER OT_SUPERLU_SOURCES EXCLUDE REGEX ".*_test\\.c$")

file(GLOB OT_SUPERLU_BLAS_SOURCES ${OT_THIRDPARTY_ROOT}/superlu/SuperLU_4.1/CBLAS/*.c)

add_library(ot_superlu STATIC ${OT_SUPERLU_SOURCES} ${OT_SUPERLU_BLAS_SOURCES})
target_include_directories(ot_superlu
    PUBLIC ${OT_THIRDPARTY_ROOT}/superlu/SuperLU_4.1/SRC
    PRIVATE ${OT_THIRDPARTY_ROOT}/superlu/SuperLU_4.1
)
target_compile_definitions(ot_superlu PRIVATE -DUSE_VENDOR_BLAS=0 -DAdd_ -DNO_TIMER)
set(SUPERLU_LIB ot_superlu)

#-----------------------------------------------------------------------------
# kiss_fft (used by several standard effects)
#-----------------------------------------------------------------------------

add_library(ot_kissfft STATIC
    ${OT_THIRDPARTY_ROOT}/kiss_fft/kiss_fft.c
    ${OT_THIRDPARTY_ROOT}/kiss_fft/kiss_fftnd.c
)
target_include_directories(ot_kissfft PUBLIC ${OT_THIRDPARTY_ROOT}/kiss_fft)
target_compile_definitions(ot_kissfft PRIVATE -Dkiss_fft_scalar=float)

#-----------------------------------------------------------------------------
# libmypaint
#
# The bundled copy is a Windows build (headers plus a DLL import library), so
# it cannot be linked on Android.  The brush engine is an optional raster
# styling feature: the MyPaint brush *files* are still read and written by the
# core (the JSON parser is part of toonzlib), only the simulation is skipped
# when the library is not present.  The Android build therefore defines
# WITHOUT_MYPAINT and toonzlib falls back to the built-in raster brush.
#-----------------------------------------------------------------------------

add_definitions(-DWITHOUT_MYPAINT)

#-----------------------------------------------------------------------------
# TinyEXR
#-----------------------------------------------------------------------------

list(APPEND OT_TP_INCLUDE_DIRS ${OT_THIRDPARTY_ROOT}/tinyexr)

#-----------------------------------------------------------------------------
# Summary
#-----------------------------------------------------------------------------

set(OT_TP_LIBRARIES
    ot_zlib
    ot_png
    ot_jpeg
    ot_tiff
    ot_lz4
    ot_lzo
    ot_superlu
    ot_kissfft
)

message(STATUS "Android third party libraries: ${OT_TP_LIBRARIES}")
