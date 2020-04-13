set(SOURCES cbor.c cbor/streaming.c cbor/internal/encoders.c cbor/internal/builder_callbacks.c cbor/internal/loaders.c cbor/internal/memory_utils.c cbor/internal/stack.c cbor/internal/unicode.c cbor/encoding.c cbor/serialization.c cbor/arrays.c cbor/common.c cbor/floats_ctrls.c cbor/bytestrings.c cbor/callbacks.c cbor/strings.c cbor/maps.c cbor/tags.c cbor/ints.c)

include(GNUInstallDirs)
set(CMAKE_SKIP_BUILD_RPATH FALSE)

if (CBOR_CUSTOM_ALLOC)
    LIST(APPEND SOURCES allocators.c)
endif(CBOR_CUSTOM_ALLOC)

if (NOT DEFINED CMAKE_MACOSX_RPATH)
	set(CMAKE_MACOSX_RPATH 0)
endif()

add_library(cbor STATIC ${SOURCES})
add_library(cbor_shared SHARED ${SOURCES})

set_target_properties(cbor_shared PROPERTIES OUTPUT_NAME cbor VERSION ${CBOR_VERSION} SOVERSION ${CBOR_VERSION})

configure_file(libcbor.pc.in libcbor.pc @ONLY)

#http://www.cmake.org/Wiki/CMake:Install_Commands

install(TARGETS cbor_shared
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION bin)

install(DIRECTORY cbor DESTINATION include
                FILES_MATCHING PATTERN "*.h")

install(FILES cbor.h DESTINATION include)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libcbor.pc"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig")
