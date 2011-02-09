# $NetBSD: Makefile,v 1.12 2011/02/09 14:32:45 pooka Exp $

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
