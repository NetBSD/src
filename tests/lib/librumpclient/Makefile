#	$NetBSD: Makefile,v 1.2 2011/02/15 15:16:46 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librumpclient

TESTS_C=	t_fd
TESTS_C+=	h_exec

TESTS_SH=	t_exec

# what do you mean I missed?
LINKS+=		${TESTSDIR}/h_exec ${TESTSDIR}/h_ution

ATFFILE=	yes

LDADD+= -lrumpclient

.include <bsd.test.mk>
