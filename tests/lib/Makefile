# $NetBSD: Makefile,v 1.15.2.1 2011/11/10 14:31:51 yamt Exp $

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
