#	$NetBSD: Makefile,v 1.2.12.1 2014/08/20 00:04:47 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/sysmon

TESTS_C=	t_swwdog

LDADD+=	-lrumpdev_sysmon -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser
LDADD+=	-lrump
LDADD+=	-lpthread

WARNS=	4

TESTS_SH=	t_swsensor

.include <bsd.test.mk>
