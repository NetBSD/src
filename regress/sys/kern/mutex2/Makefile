#	$NetBSD: Makefile,v 1.1 2007/01/17 20:56:49 ad Exp $

KMOD=   mutex2_test

.PATH:	../lkmcommon

SRCS=	lkminit_test.c test_mutex2.c
CPPFLAGS+=-DLKMENTRY=mutex2_test_lkmentry

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
