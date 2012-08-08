# $NetBSD: Makefile,v 1.20 2012/08/08 13:57:06 christos Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libbluetooth libc libcrypt libcurses libevent libexecinfo \
		libm libobjc libposix libppath libprop libpthread \
		librt libtre libutil semaphore

.if (${MKRUMP} != "no")
TESTS_SUBDIRS+= librumpclient librumphijack
.endif

.if ${MKCRYPTO} != "no"
TESTS_SUBDIRS+= libdes
.endif

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
