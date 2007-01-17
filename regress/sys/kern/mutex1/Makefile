#	$NetBSD: Makefile,v 1.1 2007/01/17 20:56:49 ad Exp $

KMOD=   mutex1_test

.PATH:	../lkmcommon

SRCS=	lkminit_test.c test_mutex1.c
CPPFLAGS+=-DLKMENTRY=test_mutex1_lkmentry

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
