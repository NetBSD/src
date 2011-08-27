# $NetBSD: Makefile,v 1.15 2011/08/27 18:55:51 dyoung Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libbluetooth libc libcurses libevent libm \
		libobjc libposix libppath libprop libpthread \
		librt librumpclient librumphijack libutil semaphore

.if ${MKCRYPTO} != "no"
TESTS_SUBDIRS+= libdes
.endif

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
