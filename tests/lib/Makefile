# $NetBSD: Makefile,v 1.9 2010/08/25 16:46:36 jmmv Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libc libevent libobjc libposix libprop librt libpthread \
		libutil semaphore

.if ${MKCRYPTO} != "no"
TESTS_SUBDIRS+= libdes
.endif

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
