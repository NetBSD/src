#	$NetBSD: Makefile,v 1.1 2007/01/17 20:56:49 ad Exp $

KMOD=   rwlock1_test

.PATH:	../lkmcommon

SRCS=	lkminit_test.c test_rwlock1.c
CPPFLAGS+=-DLKMENTRY=rwlock1_test_lkmentry

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
