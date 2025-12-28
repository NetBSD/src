# $NetBSD: Makefile,v 1.38 2025/12/28 16:50:22 thorpej Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libarchive libc libcrypt libcurses \
		libexecinfo libi386 libm libnvmm libobjc libposix libppath \
		libprop libpthread librefuse librt libstdc++ libtre \
		libutil libossaudio lua semaphore

TESTS_SUBDIR_INSTALL_ONLY=	libevent

.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_SUBDIRS+= librumpclient librumphijack
.endif

TESTS_SUBDIRS+= libdes

.if ${MKKYUA} != "no"
# These tests come from external/bsd/lutok/tests/lib/liblutok.  Ensure they
# get registered in the Atffile only when installed.
ATFFILE_EXTRA_TPS=	liblutok
.endif

.if (${MKBLUETOOTH} != "no")
TESTS_SUBDIRS+= libbluetooth
.endif

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

.if ${MKSLJIT} != "no"
TESTS_SUBDIRS+=	libsljit
TESTS_SUBDIRS+=	libbpfjit
.endif

.if (${MKUSB} != "no")
TESTS_SUBDIRS+= libusbhid
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
