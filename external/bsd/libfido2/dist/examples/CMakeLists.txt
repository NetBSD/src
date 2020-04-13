# Copyright (c) 2018 Yubico AB. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

list(APPEND COMPAT_SOURCES
	../openbsd-compat/getopt_long.c
	../openbsd-compat/strlcat.c
	../openbsd-compat/strlcpy.c
)

if(WIN32 AND NOT CYGWIN AND NOT MSYS)
	list(APPEND COMPAT_SOURCES ../openbsd-compat/posix_win.c)
endif()

# drop -rdynamic
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# manifest
add_executable(manifest manifest.c ${COMPAT_SOURCES})
target_link_libraries(manifest fido2)

# info
add_executable(info info.c ${COMPAT_SOURCES})
target_link_libraries(info fido2)

# reset
add_executable(reset reset.c util.c ${COMPAT_SOURCES})
target_link_libraries(reset fido2)

# cred
add_executable(cred cred.c util.c ${COMPAT_SOURCES})
target_link_libraries(cred fido2)

# assert
add_executable(assert assert.c util.c ${COMPAT_SOURCES})
target_link_libraries(assert fido2)

# setpin
add_executable(setpin setpin.c ${COMPAT_SOURCES})
target_link_libraries(setpin fido2)

# retries
add_executable(retries retries.c ${COMPAT_SOURCES})
target_link_libraries(retries fido2)
