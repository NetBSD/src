#	$NetBSD: Makefile,v 1.3 2007/02/24 15:25:23 yamt Exp $

KMOD=   mutex1_test

NOMAN=

.PATH:	${.CURDIR}/../lkmcommon

SRCS=	lkminit_test.c test_mutex1.c
CPPFLAGS+=-DLKMENTRY=mutex1_test_lkmentry

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
