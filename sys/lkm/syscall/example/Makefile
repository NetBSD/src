#	$NetBSD: Makefile,v 1.1.2.2 2001/01/08 14:57:36 bouyer Exp $

.include "../Makefile.inc"

KMOD=	syscall_example

MKMAN=	no

SRCS=	lkminit_syscall.c
SRCS+=	example_syscall.c

SUBDIR+= test

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
