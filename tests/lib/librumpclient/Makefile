#	$NetBSD: Makefile,v 1.3.2.2 2011/02/17 12:00:54 bouyer Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librumpclient

TESTS_C=	t_fd
TESTS_C+=	h_exec

TESTS_SH=	t_exec

ATFFILE=	yes

LDADD+= -lrumpclient

.include <bsd.test.mk>
