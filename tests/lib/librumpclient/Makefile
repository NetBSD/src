#	$NetBSD: Makefile,v 1.6 2016/01/23 21:22:49 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librumpclient

TESTS_C=	t_fd

TESTS_SH=	t_exec

BINDIR=		${TESTSDIR}
MKMAN=		no

PROGS+=		h_exec
PROGS+=		h_execthr
CPPFLAGS+=	-D_KERNTYPES

LDADD+= -lrumpclient
LDADD.h_execthr= -lpthread

.include <bsd.test.mk>
