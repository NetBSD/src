#	$NetBSD: Makefile,v 1.1 2008/03/28 20:44:57 ad Exp $

KMOD=   callout1_test

NOMAN=

.PATH:	${.CURDIR}/../lkmcommon

SRCS=	lkminit_test.c test_callout1.c
CPPFLAGS+=-DLKMENTRY=callout1_test_lkmentry

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
