# $NetBSD: Makefile,v 1.11 2011/02/06 18:44:29 pooka Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libc libm libevent libobjc libposix libprop librt \
		libpthread librumphijack libutil semaphore

.if ${MKCRYPTO} != "no"
TESTS_SUBDIRS+= libdes
.endif

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
