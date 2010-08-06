#	$NetBSD: Makefile,v 1.1 2010/08/06 16:13:26 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/sysmon

TESTS_C=	t_swwdog

LDADD+=	-lrumpdev_sysmon -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>
