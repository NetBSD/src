#!/bin/bash -u

# Copyright (c) 2019 Yubico AB. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

OPENSSH=$(realpath ../../openssh)
LIBRESSL=$(realpath ../../libressl-2.8.3)
[[ ! -d "${OPENSSH}" || ! -d "${LIBRESSL}" ]] && exit 1

diff -pu bsd-getpagesize.c ${OPENSSH}/openbsd-compat/bsd-getpagesize.c
diff -pu err.h ${LIBRESSL}/include/compat/err.h
diff -pu explicit_bzero.c ${OPENSSH}/openbsd-compat/explicit_bzero.c
diff -pu explicit_bzero_win32.c ${LIBRESSL}/crypto/compat/explicit_bzero_win.c
diff -pu getopt.h ${OPENSSH}/openbsd-compat/getopt.h
diff -pu getopt_long.c ${OPENSSH}/openbsd-compat/getopt_long.c
diff -pu posix_win.c ${LIBRESSL}/crypto/compat/posix_win.c
diff -pu readpassphrase.c ${OPENSSH}/openbsd-compat/readpassphrase.c
diff -pu readpassphrase.h ${OPENSSH}/openbsd-compat/readpassphrase.h
diff -pu recallocarray.c ${OPENSSH}/openbsd-compat/recallocarray.c
diff -pu strlcat.c ${OPENSSH}/openbsd-compat/strlcat.c
diff -pu strlcpy.c ${OPENSSH}/openbsd-compat/strlcpy.c
diff -pu timingsafe_bcmp.c ${OPENSSH}/openbsd-compat/timingsafe_bcmp.c
diff -pu types.h ${LIBRESSL}/include/compat/sys/types.h
