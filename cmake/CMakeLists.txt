# Boilerplate.
cmake_minimum_required (VERSION 3.1 FATAL_ERROR)   # First version with CMAKE_CXX_STANDARD.
project (skimake CXX)
set (CMAKE_CXX_STANDARD 11)

link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)

# Default to Release mode.  We're mainly targeting Skia users, not Skia developers.
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif ()

# we don't need this option for static libraries
if(NOT(CMAKE_VERSION LESS 3.4) AND MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    message( STATUS "CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS: ${CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS}" )
endif()

# To first approximation, the Skia library comprises all .cpp files under src/.
file (GLOB_RECURSE srcs ../src/*.cpp)

function (find_include_dirs out)
    file (GLOB_RECURSE headers ${ARGN})
    foreach (path ${headers})
        get_filename_component (dir ${path} PATH)
        list (APPEND include_dirs ${dir})
    endforeach()
    list (REMOVE_DUPLICATES include_dirs)
    set (${out} ${include_dirs} PARENT_SCOPE)
endfunction()

# We need src/ directories and include/private on our path when building Skia.
# Users should be able to use Skia with only include/ directories that are not include/private.
find_include_dirs(private_includes ../src/*.h ../include/private/*.h)
find_include_dirs(public_includes ../include/*.h)
list (REMOVE_ITEM public_includes ${private_includes})  # Easiest way to exclude private.
file (GLOB default_include_config "../include/config")
list (REMOVE_ITEM public_includes ${default_include_config})
set (userconfig_directory ${CMAKE_BINARY_DIR}/include)
list (APPEND public_includes ${userconfig_directory})

# These guys are third_party but provided by a Skia checkout.
list (APPEND srcs             ../third_party/etc1/etc1.cpp ../third_party/ktx/ktx.cpp)
list (APPEND private_includes ../third_party/etc1          ../third_party/ktx)
list (APPEND private_includes ${CMAKE_CURRENT_BINARY_DIR}/include)

function (remove_srcs)
    file (GLOB_RECURSE to_remove ${ARGN})
    list (REMOVE_ITEM srcs ${to_remove})
    set (srcs ${srcs} PARENT_SCOPE)
endfunction()

# TODO: after create ExternalProjects for all DEPS we can comment 'remove_src' force_link sources
# This file is empty and is only used to trick GYP.
remove_srcs (../src/core/SkForceCPlusPlusLinking.cpp)
# This file forces linking for all our supported image decoders.  We're more fine-grained.
remove_srcs (../src/images/SkForceLinking.cpp)
# Chrome only?
remove_srcs (../src/ports/SkFontHost_fontconfig.cpp
             ../src/fonts/SkFontMgr_fontconfig.cpp
             ../src/ports/SkFontConfigInterface_direct.cpp
             ../src/ports/SkFontConfigInterface_direct_factory.cpp
             ../src/ports/SkFontConfigInterface_direct_google3.cpp
             ../src/ports/SkFontConfigInterface_direct_google3_factory.cpp)
# Alternative font managers.
remove_srcs (../src/ports/SkFontMgr_custom*.cpp)

# Skia sure ships a lot of code no one uses.
remove_srcs (../src/animator/* ../src/*nacl* ../src/svg/* ../src/views/* ../src/xml/*)
foreach (include animator svg svg/parser views xml gpu/vk)
  file (GLOB globed_include ../include/${include})
  list (REMOVE_ITEM public_includes ${globed_include})
endforeach()

# Remove OS-specific source files.
if (NOT UNIX)
    remove_srcs(../src/ports/*_posix.cpp
                ../src/ports/SkTLS_pthread.cpp
                ../src/ports/SkTime_Unix.cpp
                ../src/utils/SkThreadUtils_pthread.cpp)
endif()

if (APPLE OR NOT UNIX)
    remove_srcs(../src/gpu/gl/glx/*
                ../src/ports/SkFontMgr_fontconfig*.cpp
                ../src/ports/SkFontMgr_android*.cpp
                ../src/*FreeType*)
endif()

# Remove processor-specific source files.
if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL ARM)
    remove_srcs(../src/*arm* ../src/*ARM* ../src/*neon* ../src/*NEON*)
endif()
if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL MIPS)
    remove_srcs(../src/*mips* ../src/*MIPS*)
endif()

# Make our ports choices.
remove_srcs(
    ../src/*moz*   # We're probably not Mozilla.
    ../src/gpu/GrContextFactory.cpp                  # For internal testing only.
    ../src/gpu/gl/GrGLCreateNativeInterface_none.cpp
    ../src/gpu/gl/GrGLDefaultInterface_none.cpp
    ../src/gpu/gl/command_buffer/*
    ../src/gpu/gl/egl/*
    ../src/gpu/gl/glfw/*
    ../src/gpu/gl/iOS/*
    ../src/gpu/vk/*
    ../src/opts/SkBitmapProcState_opts_none.cpp
    ../src/opts/SkBlitMask_opts_none.cpp
    ../src/opts/SkBlitRow_opts_none.cpp
    ../src/ports/SkFontMgr_empty_factory.cpp
    ../src/ports/SkGlobalInitialization_chromium.cpp
    ../src/ports/SkImageEncoder_none.cpp
    ../src/ports/SkImageGenerator_none.cpp
    ../src/ports/SkTLS_none.cpp)

if (NOT APPLE)
    remove_srcs(../src/ports/SkImageGeneratorCG.cpp)
endif()

if (NOT WIN32)
    remove_srcs(../src/ports/SkImageGeneratorWIC.cpp)
endif()

if (WIN32)
    if(SKIA_GDI)
        message( STATUS "SKIA use GDI" )
        remove_srcs(../src/ports/SkFontMgr_win_dw_factory.cpp)
    else()
        message( STATUS "SKIA use DirectWrite" )
        remove_srcs(../src/ports/SkFontMgr_win_gdi_factory.cpp)
    endif()
endif()

# Certain files must be compiled with support for SSSE3, SSE4.1, AVX, or AVX2 intrinsics.
file (GLOB_RECURSE ssse3_srcs ../src/*ssse3*.cpp ../src/*SSSE3*.cpp)
file (GLOB_RECURSE sse41_srcs ../src/*sse4*.cpp ../src/*SSE4*.cpp)
file (GLOB_RECURSE avx_srcs   ../src/*_avx.cpp)
file (GLOB_RECURSE avx2_srcs  ../src/*_avx2.cpp)

if (MSVC)
set_source_files_properties(${ssse3_srcs} PROPERTIES COMPILE_FLAGS /arch:SSE2)
set_source_files_properties(${ssse3_srcs} PROPERTIES COMPILE_DEFINITIONS  SK_CPU_SSE_LEVEL=31)
set_source_files_properties(${sse41_srcs} PROPERTIES COMPILE_FLAGS /arch:SSE2)
set_source_files_properties(${sse41_srcs} PROPERTIES COMPILE_DEFINITIONS SK_CPU_SSE_LEVEL=42)
set_source_files_properties(${avx_srcs}   PROPERTIES COMPILE_FLAGS /arch:AVX)
set_source_files_properties(${avx2_srcs}  PROPERTIES COMPILE_FLAGS /arch:AVX2)
else()
set_source_files_properties(${ssse3_srcs} PROPERTIES COMPILE_FLAGS -mssse3)
set_source_files_properties(${sse41_srcs} PROPERTIES COMPILE_FLAGS -msse4.1)
set_source_files_properties(${avx_srcs}   PROPERTIES COMPILE_FLAGS -mavx)
set_source_files_properties(${avx2_srcs}  PROPERTIES COMPILE_FLAGS -mavx2)
endif()

include( ExternalProject )

# Detect our optional dependencies.
# If we can't find them, don't build the parts of Skia that use them.
find_package (EXPAT)
find_package (Lua)
find_package (ZLIB)

# No find_package for libwebp as far as I can tell, so simulate it here.
find_path (WEBP_INCLUDE_DIRS "webp/decode.h")
find_library (WEBP_LIBRARIES webp)
find_path (OSMESA_INCLUDE_DIRS "GL/osmesa.h")
find_library(OSMESA_LIBRARIES "OSMesa")

if (NOT APPLE)
    find_package (Freetype)
    # Same deal for fontconfig.
    find_path (FONTCONFIG_INCLUDE_DIRS "fontconfig/fontconfig.h")
    find_library (FONTCONFIG_LIBRARIES fontconfig)
    find_package (GIF)
    find_package (JPEG)
    find_package (PNG)
endif()

# Do not compile SkRawCodec.
remove_srcs(../src/codec/*Raw*.cpp)
set(skia_dependecies)

# TODO: macro away this if (found) ... else() ... endif() stuff.

if (EXPAT_FOUND)
    list (APPEND private_includes ${EXPAT_INCLUDE_DIRS})
    list (APPEND libs             ${EXPAT_LIBRARIES})
else()
    remove_srcs (../src/ports/SkFontMgr_android_parser.cpp)
endif()

if (GIF_FOUND)
    list (APPEND private_includes ${GIF_INCLUDE_DIRS})
    list (APPEND libs             ${GIF_LIBRARIES})
    add_definitions(-DSK_CODEC_DECODES_GIF)
else()
    remove_srcs(../src/images/*GIF*)
    remove_srcs(../src/codec/*Gif*)
endif()

if (JPEG_FOUND)
    list (APPEND private_includes ${JPEG_INCLUDE_DIRS})
    list (APPEND libs             ${JPEG_LIBRARIES})
    add_definitions(-DSK_CODEC_DECODES_JPEG)
else()
    remove_srcs(../src/images/*JPEG*)
    remove_srcs(../src/codec/*Jpeg*)
endif()

if (LUA_FOUND)
    list (APPEND private_includes ${LUA_INCLUDE_DIR})
    list (APPEND libs             ${LUA_LIBRARIES})
else()
    remove_srcs(../src/utils/*Lua*)
endif()

if (NOT ZLIB_FOUND)
    message( STATUS "Will build ZLIB from external sources" )

    set( zlib_source_path ${CMAKE_CURRENT_BINARY_DIR}/external/zlib/src/external_zlib/ )
    set( zlib_cmake_file_path ${zlib_source_path}/CMakeLists.txt )
    
    ExternalProject_Add( external_zlib
                    PREFIX            ${CMAKE_CURRENT_BINARY_DIR}/external/zlib
                    INSTALL_DIR       ${CMAKE_CURRENT_BINARY_DIR}
                    # http://www.zlib.net/
                    URL http://zlib.net/zlib-1.2.8.tar.gz
                    # GIT_REPOSITORY    git@github.com:madler/zlib.git
                    # GIT_TAG           v1.2.8
                    CMAKE_ARGS        -DASM686=off
                                      -DAMD64=off
                                      -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
                                      -DCMAKE_COMPILER_IS_GNUCXX=${CMAKE_COMPILER_IS_GNUCXX}
                                      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                      -DCMAKE_C_FLAGS=${GL_BINDING_C_FLAGS}
                                      -DBUILD_SHARED_LIBS=FALSE
    )
    
    set(ZLIB_FOUND TRUE)
    
    add_library(ZLibStatic STATIC IMPORTED GLOBAL)
    add_dependencies(ZLibStatic external_zlib)
    if(MSVC)
        set_target_properties(ZLibStatic PROPERTIES
                            IMPORTED_LOCATION_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/lib/zlibstaticd.lib
                            IMPORTED_LOCATION_MINSIZEREL ${CMAKE_CURRENT_BINARY_DIR}/lib/zlibstatic.lib
                            IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/lib/zlibstatic.lib
                            IMPORTED_LOCATION_RELWITHDEBINFO ${CMAKE_CURRENT_BINARY_DIR}/lib/zlibstatic.lib
                            LINKER_LANGUAGE C)
    else()
        set_target_properties(ZLibStatic PROPERTIES
                            IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/libz.a
                          LINKER_LANGUAGE C)
    endif()

    list (APPEND libs ZLibStatic)
    #list (APPEND skia_dependecies external_zlib)
else()    
    list (APPEND libs ${ZLIB_LIBRARIES})
    list (APPEND private_includes ${ZLIB_INCLUDE_DIRS})
endif()

remove_srcs(../src/pdf/SkDocument_PDF_None.cpp)

if (NOT PNG_FOUND)
    message( STATUS "Will build PNG from external sources" )
    
    set( libpng_source_path ${CMAKE_CURRENT_BINARY_DIR}/external/libpng/src/external_libpng/ )
    set( libpng_cmake_file_path ${libpng_source_path}/CMakeLists.txt )    
    
    ExternalProject_Add( external_libpng
    DEPENDS           external_zlib
    PREFIX            ${CMAKE_CURRENT_BINARY_DIR}/external/libpng
    INSTALL_DIR       ${CMAKE_CURRENT_BINARY_DIR}
    # http://www.libpng.org/pub/png/libpng.html
    #URL ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/lpng1621.zip
    URL http://download.sourceforge.net/libpng/lpng1621.zip
    #GIT_REPOSITORY    git://git.code.sf.net/p/libpng/code
    #GIT_TAG           v1.6.9
    CMAKE_ARGS          -DPNG_SHARED=off
                        -DPNG_STATIC=on
                        -DPNG_TESTS=off
                        -DPNG_DEBUG=off
                        -DPNGARG=off
                        -DBUILD_SHARED_LIBS=FALSE
                        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
                        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                        -DCMAKE_COMPILER_IS_GNUCXX=${CMAKE_COMPILER_IS_GNUCXX}
                        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                        -DCMAKE_C_FLAGS=${GL_BINDING_C_FLAGS}
    )
    
    set(PNG_FOUND TRUE)
    add_library(libpng STATIC IMPORTED GLOBAL)
    add_dependencies(libpng external_zlib external_libpng)
    if(MSVC)
        set_target_properties(libpng PROPERTIES
                            IMPORTED_LOCATION_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/lib/libpng16_staticd.lib
                            IMPORTED_LOCATION_MINSIZEREL ${CMAKE_CURRENT_BINARY_DIR}/lib/libpng16_static.lib
                            IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/lib/libpng16_static.lib
                            IMPORTED_LOCATION_RELWITHDEBINFO ${CMAKE_CURRENT_BINARY_DIR}/lib/libpng16_static.lib
                            LINKER_LANGUAGE C)
    else()
        set_target_properties(libpng PROPERTIES
                            IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib/png16_static.a
                          LINKER_LANGUAGE C)
    endif()
    
    
    #list (APPEND skia_dependecies external_libpng)
    list (APPEND libs libpng)
else()
    list (APPEND private_includes ${PNG_INCLUDE_DIRS})
    list (APPEND libs             ${PNG_LIBRARIES})
endif()

add_definitions(-DPNG_SKIP_SETJMP_CHECK)
add_definitions(-DPNG_SKIP_SKIA_OPTS)
add_definitions(-DSK_CODEC_DECODES_PNG)

if (WEBP_INCLUDE_DIRS AND WEBP_LIBRARIES)
    list (APPEND private_includes ${WEBP_INCLUDE_DIRS})
    list (APPEND libs             ${WEBP_LIBRARIES})
    add_definitions(-DSK_CODEC_DECODES_WEBP)
else()
    remove_srcs(../src/images/*WEBP*)
    remove_srcs(../src/codec/*Webp*)
endif()

if (FREETYPE_FOUND)
    list (APPEND private_includes ${FREETYPE_INCLUDE_DIRS})
    list (APPEND libs             ${FREETYPE_LIBRARIES})
endif()

if (FONTCONFIG_INCLUDE_DIRS AND FONTCONFIG_LIBRARIES)
    list (APPEND private_includes ${FONTCONFIG_INCLUDE_DIRS})
    list (APPEND libs             ${FONTCONFIG_LIBRARIES})
endif()

if (APPLE)
    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices REQUIRED)
    list (APPEND libs ${APPLICATION_SERVICES_FRAMEWORK})
endif()

if (OSMESA_LIBRARIES AND OSMESA_INCLUDE_DIRS)
    list (APPEND libs             ${OSMESA_LIBRARIES})
    list (APPEND private_includes ${OSMESA_INCLUDE_DIRS})
    list (APPEND public_defines   "-DSK_MESA=1")
    set (SK_MESA 1)
else()
    remove_srcs(../src/gpu/gl/mesa/*)
endif()

if (WIN32)
    list (APPEND libs FontSub.lib Usp10.lib gdi32.lib)
endif()

find_package(OpenGL REQUIRED)
list (APPEND libs  ${OPENGL_LIBRARIES})

#add_definitions(-DSK_SUPPORT_LEGACY_PATHEFFECT_PTR)
#add_definitions(-DSK_SUPPORT_LEGACY_MASKFILTER_PTR)
#add_definitions(-DSK_SUPPORT_LEGACY_MINOR_EFFECT_PTR)


# This is our main output, libskia.so.
# We mostly build an .so here because it helps test we've linked everything,
# not so much that we think Skia is a good candidate to ship as a shared library.
add_library (skia SHARED ${srcs})
set_property(TARGET skia PROPERTY CXX_STANDARD 11)
set_property(TARGET skia PROPERTY CXX_STANDARD_REQUIRED ON)
set_target_properties(skia PROPERTIES LINKER_LANGUAGE CXX)

#if(skia_dependecies)
#    add_dependencies(skia ${skia_dependecies})
#endif()


target_compile_definitions(skia
    PUBLIC  ${public_defines}
    PRIVATE -DSKIA_DLL -DSKIA_IMPLEMENTATION=1
)

target_include_directories(skia
    PUBLIC  ${public_includes}
    PRIVATE ${private_includes})

target_link_libraries(skia
    PUBLIC
    PRIVATE ${libs})

message(STATUS "SKIA external libraries: " )    
foreach(lib ${libs})
		message(STATUS " link to ${lib}" )    
endforeach()

if (MSVC)
	target_compile_definitions(skia 
			PRIVATE "$<$<CONFIG:RELEASE>:/DNDEBUG>"
			PRIVATE "$<$<CONFIG:MINSIZEREL>:/DNDEBUG>"
	)
	target_compile_options(skia PRIVATE "/MD$<$<CONFIG:Debug>:d>")
    set(cc_flags "/DOS_WIN /DWIN32 /D_UNICODE /DUNICODE /DGR_GL_FUNCTION_TYPE=__stdcall /D_CRT_SECURE_NO_WARNINGS /DSK_BUILD_FOR_WIN32 /wd4275 /wd4345 /wd4355 /wd4251 /GR-")
else()
    set(cc_flags "-w -fno-rtti -fno-exceptions")
endif()

set_target_properties(skia PROPERTIES
    COMPILE_FLAGS ${cc_flags}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN true)

# Experimental C API install:
file(GLOB c_headers "../include/c/*.h")
install(FILES ${c_headers} DESTINATION include)
install(TARGETS skia DESTINATION lib)

# SkUserConfig.h
if (CMAKE_BUILD_TYPE STREQUAL Release)
    set (SK_RELEASE 1)
else()
    set (SK_RELEASE 0)
endif()

if (UNIX AND NOT APPLE)
  set (SK_SAMPLES_FOR_X 1)
else()
  set (SK_SAMPLES_FOR_X 0)
endif()

configure_file ("SkUserConfig.h.in" "${userconfig_directory}/SkUserConfig.h")

# skia_link_arguments.txt
set (link_arguments ${CMAKE_BINARY_DIR}/skia_link_arguments.txt)
file (WRITE ${link_arguments} "-L${CMAKE_BINARY_DIR}\n")
file (APPEND ${link_arguments} "-lskia\n")
file (APPEND ${link_arguments} "-Wl,-rpath,${CMAKE_BINARY_DIR}\n")

# skia_compile_arguments.txt
set (compile_arguments ${CMAKE_BINARY_DIR}/skia_compile_arguments.txt)

if (UNIX)
	file (WRITE ${compile_arguments} "--std=c++11\n")
endif()

foreach (include ${public_includes})
  get_filename_component (abs_include ${include} ABSOLUTE)
  file (APPEND ${compile_arguments} "-I${abs_include}\n")
endforeach()

# cmake .
# cmake --build . --target skia
# c++ -c @skia_compile_arguments.txt example.cpp
# c++ example.o @skia_link_arguments.txt

# skia.h
set (bad_files GrGLConfig_chrome.h SkJSONCPP.h SkParsePaint.h)

# make `c++ @skia_compile_arguments.txt include/skia.h` work.
set (skia_h_path ${userconfig_directory}/skia.h)
file (WRITE ${skia_h_path} "// skia.h generated by CMake.\n")
file(APPEND ${skia_h_path} "#ifndef skia_DEFINED\n")
file(APPEND ${skia_h_path} "#define skia_DEFINED\n")
foreach (include ${public_includes})
  if (NOT include STREQUAL userconfig_directory)
    file (APPEND ${skia_h_path} "\n")
    file (GLOB all_public_headers ${include}/*.h)
    foreach (public_header ${all_public_headers})
      get_filename_component (filename_component ${public_header} NAME)
      if (NOT ";${bad_files};" MATCHES ";${filename_component};")
        file (APPEND ${skia_h_path} "#include \"${filename_component}\"\n")
      endif ()
    endforeach()
  endif()
endforeach()
file(APPEND ${skia_h_path} "\n#endif  // skia_DEFINED\n")

# Now build a simple example app that uses Skia via libskia.so.
add_executable(example example.cpp)
#add_dependencies(example skia)
target_link_libraries(example skia ${OPENGL_LIBRARIES})

# Now build a experimental app
#file (GLOB_RECURSE pdfView ../experimental/PdfViewer/*.cpp)


#add_executable(pdf2bmp ${pdfView} 
#    ../src/core/SkOpts.cpp
#    ../src/opts/SkOpts_ssse3.cpp
#    ../src/opts/SkOpts_sse41.cpp
#    ../src/core/SkHalf.cpp 
#    ../src/core/SkTSearch.cpp 
#    ../src/core/SkChunkAlloc.cpp 
#    ../src/core/SkUtils.cpp
#    ../src/codec/SkCodecImageGenerator.cpp
#    ../src/codec/SkCodec.cpp
#    ../src/codec/SkSampler.cpp
#    ../src/codec/SkMasks.cpp 
#    ../src/codec/SkSwizzler.cpp
#    ../src/codec/SkMaskSwizzler.cpp 
#    ../src/codec/SkBmpCodec.cpp
#    ../src/codec/SkBmpMaskCodec.cpp
#    ../src/codec/SkBmpRLECodec.cpp
#    ../src/codec/SkBmpStandardCodec.cpp
#    ../src/codec/SkWbmpCodec.cpp 
#)
    
#add_dependencies(pdf2bmp skia)
#target_include_directories(pdf2bmp PRIVATE
#    ../experimental/PdfViewer
#    ../experimental/PdfViewer/inc
#    ../experimental/PdfViewer/src
#    ../experimental/PdfViewer/pdfparser
#    ../experimental/PdfViewer/pdfparser/native
#    ../experimental/PdfViewer/pdfparser/native/pdfapi
#    ${private_includes}
#    )
#target_link_libraries(pdf2bmp skia)
