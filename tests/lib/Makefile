# $NetBSD: Makefile,v 1.10.2.2 2011/02/17 12:00:54 bouyer Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libc libm libevent libobjc libposix libprop librt \
		libpthread librumpclient librumphijack libutil semaphore

.if ${MKCRYPTO} != "no"
TESTS_SUBDIRS+= libdes
.endif

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
