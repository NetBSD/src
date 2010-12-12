#	$NetBSD: Makefile,v 1.2 2010/12/12 12:53:35 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern/h_client

TESTS_C+=	h_simplecli
TESTS_C+=	h_stresscli

ATFFILE=	no

LDADD+=			-lrumpclient
LDADD.h_stresscli=	-lpthread

WARNS=	4
NOMAN=

.include <bsd.test.mk>
