# $NetBSD: Makefile,v 1.16 2011/11/06 18:43:51 christos Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libbluetooth libc libcurses libevent libm \
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
