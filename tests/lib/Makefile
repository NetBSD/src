# $NetBSD: Makefile,v 1.17 2011/12/27 00:47:23 christos Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libbluetooth libc libcrypt libcurses libevent libm \
		libobjc libposix libppath libprop libpthread \
		librt librumpclient librumphijack libtre libutil semaphore

.if ${MKCRYPTO} != "no"
TESTS_SUBDIRS+= libdes
.endif

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
