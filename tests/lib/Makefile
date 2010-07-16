# $NetBSD: Makefile,v 1.5 2010/07/16 13:56:31 jmmv Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	libc libevent libposix libprop librt libutil semaphore

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
