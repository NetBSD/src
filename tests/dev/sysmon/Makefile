#	$NetBSD: Makefile,v 1.5 2020/03/01 18:08:13 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/sysmon

TESTS_C=	t_swwdog

CPPFLAGS+=	-D_KERNTYPES
LDADD+=	-lrumpdev_sysmon -lrumpdev ${LIBRUMPBASE}

WARNS=	4

TESTS_SH=	t_swsensor

.include <bsd.test.mk>
