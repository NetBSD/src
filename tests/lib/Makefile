# $NetBSD: Makefile,v 1.21 2012/11/05 00:57:42 alnsn Exp $

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

.if ${MKSLJIT} != "no"
TESTS_SUBDIRS+=	libsljit
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
