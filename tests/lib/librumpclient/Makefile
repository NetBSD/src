#	$NetBSD: Makefile,v 1.4 2011/03/08 12:40:25 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librumpclient

TESTS_C=	t_fd
TESTS_C+=	h_exec
TESTS_C+=	h_execthr

TESTS_SH=	t_exec

ATFFILE=	yes

LDADD+= -lrumpclient
LDADD.h_execthr= -lpthread

.include <bsd.test.mk>
