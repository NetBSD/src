# $NetBSD: Makefile,v 1.8 2010/07/18 12:41:51 jmmv Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libc libevent libobjc libposix libprop librt libpthread \
		libutil semaphore

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
