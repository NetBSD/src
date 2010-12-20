# $NetBSD: Makefile,v 1.10 2010/12/20 23:47:23 pgoyette Exp $

.include <bsd.own.mk>

TESTS_SUBDIRS=	csu libc libm libevent libobjc libposix libprop librt \
		libpthread libutil semaphore

.if ${MKCRYPTO} != "no"
TESTS_SUBDIRS+= libdes
.endif

.if (${MKSKEY} != "no")
TESTS_SUBDIRS+= libskey
.endif

TESTSDIR=	${TESTSBASE}/lib

.include <bsd.test.mk>
