# $NetBSD: Makefile,v 1.6 2010/07/16 15:42:53 jmmv Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	libc libevent libposix libprop librt libpthread libutil \
		semaphore

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
