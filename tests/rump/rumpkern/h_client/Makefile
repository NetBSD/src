#	$NetBSD: Makefile,v 1.1 2010/11/30 22:09:15 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern/h_client

TESTS_C+=	h_reboot
TESTS_C+=	h_simplecli
TESTS_C+=	h_stresscli

ATFFILE=	no

LDADD+=			-lrumpclient
LDADD.h_stresscli=	-lpthread

WARNS=	4
NOMAN=

.include <bsd.test.mk>
