# $NetBSD: Makefile,v 1.13 2011/04/07 06:24:07 plunky Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libbluetooth libc libevent libm \
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
