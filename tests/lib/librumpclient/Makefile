#	$NetBSD: Makefile,v 1.3 2011/02/16 16:02:52 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librumpclient

TESTS_C=	t_fd
TESTS_C+=	h_exec

TESTS_SH=	t_exec

ATFFILE=	yes

LDADD+= -lrumpclient

.include <bsd.test.mk>
