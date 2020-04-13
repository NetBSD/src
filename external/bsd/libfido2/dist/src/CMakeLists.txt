# Copyright (c) 2018 Yubico AB. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

add_definitions(-D_FIDO_INTERNAL)

list(APPEND FIDO_SOURCES
	aes256.c
	assert.c
	authkey.c
	bio.c
	blob.c
	buf.c
	cbor.c
	cred.c
	credman.c
	dev.c
	ecdh.c
	eddsa.c
	err.c
	es256.c
	hid.c
	info.c
	io.c
	iso7816.c
	log.c
	pin.c
	reset.c
	rs256.c
	u2f.c
)

if(FUZZ)
	list(APPEND FIDO_SOURCES ../fuzz/prng.c)
	list(APPEND FIDO_SOURCES ../fuzz/uniform_random.c)
	list(APPEND FIDO_SOURCES ../fuzz/wrap.c)
endif()

if(USE_HIDAPI)
	list(APPEND COMPAT_SOURCES hid_hidapi.c)
elseif(WIN32)
	list(APPEND COMPAT_SOURCES hid_win.c)
elseif(APPLE)
	list(APPEND COMPAT_SOURCES hid_osx.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	list(APPEND COMPAT_SOURCES hid_linux.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
	list(APPEND COMPAT_SOURCES hid_openbsd.c)
else()
	message(FATAL_ERROR "please define a hid backend for your platform")
endif()

list(APPEND COMPAT_SOURCES
	../openbsd-compat/bsd-getpagesize.c
	../openbsd-compat/explicit_bzero.c
	../openbsd-compat/explicit_bzero_win32.c
	../openbsd-compat/recallocarray.c
	../openbsd-compat/strlcat.c
	../openbsd-compat/timingsafe_bcmp.c
)

# static library
add_library(fido2 STATIC ${FIDO_SOURCES} ${COMPAT_SOURCES})
target_link_libraries(fido2 ${CBOR_LIBRARIES} ${CRYPTO_LIBRARIES}
	${UDEV_LIBRARIES} ${BASE_LIBRARIES} ${HIDAPI_LIBRARIES})
if(WIN32)
	if (MINGW)
		target_link_libraries(fido2 wsock32 ws2_32 bcrypt setupapi hid)
	else()
		target_link_libraries(fido2 wsock32 ws2_32 bcrypt SetupAPI hid)
		set_target_properties(fido2 PROPERTIES OUTPUT_NAME fido2_static)
	endif()
elseif(APPLE)
	target_link_libraries(fido2 "-framework CoreFoundation"
		"-framework IOKit")
endif()
install(TARGETS fido2 ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# dynamic library
add_library(fido2_shared SHARED ${FIDO_SOURCES} ${COMPAT_SOURCES})
target_link_libraries(fido2_shared ${CBOR_LIBRARIES} ${CRYPTO_LIBRARIES}
	${UDEV_LIBRARIES} ${BASE_LIBRARIES} ${HIDAPI_LIBRARIES})
if(WIN32)
	if (MINGW)
		target_link_libraries(fido2_shared wsock32 ws2_32 bcrypt
			setupapi hid)
	else()
		target_link_libraries(fido2_shared wsock32 ws2_32 bcrypt
			SetupAPI hid)
	endif()
elseif(APPLE)
	target_link_libraries(fido2_shared "-framework CoreFoundation"
		"-framework IOKit")
endif()
set_target_properties(fido2_shared PROPERTIES OUTPUT_NAME fido2
	VERSION ${LIB_VERSION} SOVERSION ${LIB_SOVERSION})
install(TARGETS fido2_shared
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES fido.h DESTINATION include)
install(DIRECTORY fido DESTINATION include)

if(NOT WIN32)
	configure_file(libfido2.pc.in libfido2.pc @ONLY)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libfido2.pc"
		DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()
