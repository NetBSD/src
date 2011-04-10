# $NetBSD: Makefile,v 1.14 2011/04/10 09:49:13 blymn Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libbluetooth libc libcurses libevent libm \
		libobjc libposix libprop libpthread \
		librt librumpclient librumphijack libutil semaphore

.if ${MKCRYPTO} != "no"
TESTS_SUBDIRS+= libdes
.endif

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
