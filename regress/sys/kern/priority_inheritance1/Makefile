#	$NetBSD: Makefile,v 1.1 2007/02/25 09:52:47 yamt Exp $

KMOD=   priority_inheritance1_test

NOMAN=

.PATH:	${.CURDIR}/../lkmcommon

SRCS=	lkminit_test.c test_priority_inheritance1.c
CPPFLAGS+=-DLKMENTRY=priority_inheritance1_test_lkmentry

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
