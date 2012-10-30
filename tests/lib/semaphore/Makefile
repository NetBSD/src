#	$NetBSD: Makefile,v 1.2.6.1 2012/10/30 19:00:04 yamt Exp $
#
.include <bsd.own.mk>

.if (${MKRUMP} != "no")
TESTS_SUBDIRS=		pthread
.endif

TESTSDIR=	${TESTSBASE}/lib/semaphore

.include <bsd.test.mk>
