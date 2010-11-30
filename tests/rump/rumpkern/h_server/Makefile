#	$NetBSD: Makefile,v 1.1 2010/11/30 22:09:15 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern/h_server

TESTS_C=	h_simpleserver

ATFFILE=	no

LDADD+=	-lrump -lrumpuser -lpthread

WARNS=	4
NOMAN=

.include <bsd.test.mk>
