#	$NetBSD: Makefile,v 1.4.2.1 2011/02/08 16:20:10 bouyer Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern/h_client

TESTS_C+=	h_forkcli
TESTS_C+=	h_reconcli
TESTS_C+=	h_sigcli
TESTS_C+=	h_simplecli
TESTS_C+=	h_stresscli

ATFFILE=	no

LDADD+=			-lrumpclient
LDADD.h_stresscli=	-lpthread
LDADD.h_reconcli=	-lpthread

WARNS=	4
NOMAN=

.include <bsd.test.mk>
