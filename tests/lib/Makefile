# $NetBSD: Makefile,v 1.7 2010/07/17 19:25:26 jmmv Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libc libevent libposix libprop librt libpthread libutil \
		semaphore

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
