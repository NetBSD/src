#	$NetBSD: Makefile,v 1.4 2011/01/06 07:00:28 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern/h_client

TESTS_C+=	h_forkcli
TESTS_C+=	h_sigcli
TESTS_C+=	h_simplecli
TESTS_C+=	h_stresscli

ATFFILE=	no

LDADD+=			-lrumpclient
LDADD.h_stresscli=	-lpthread

WARNS=	4
NOMAN=

.include <bsd.test.mk>
