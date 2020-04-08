#	$NetBSD: Makefile,v 1.4.16.1 2020/04/08 14:09:06 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/sysmon

TESTS_C=	t_swwdog

CPPFLAGS+=	-D_KERNTYPES
LDADD+=	-lrumpdev_sysmon -lrumpdev ${LIBRUMPBASE}

WARNS=	4

TESTS_SH=	t_swsensor

.include <bsd.test.mk>
