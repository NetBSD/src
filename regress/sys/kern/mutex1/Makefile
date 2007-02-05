#	$NetBSD: Makefile,v 1.2 2007/02/05 20:18:43 ad Exp $

KMOD=   mutex1_test

.PATH:	../lkmcommon

SRCS=	lkminit_test.c test_mutex1.c
CPPFLAGS+=-DLKMENTRY=mutex1_test_lkmentry

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
