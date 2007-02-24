#	$NetBSD: Makefile,v 1.2 2007/02/24 15:25:24 yamt Exp $

KMOD=   mutex2_test

NOMAN=

.PATH:	${.CURDIR}/../lkmcommon

SRCS=	lkminit_test.c test_mutex2.c
CPPFLAGS+=-DLKMENTRY=mutex2_test_lkmentry

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
