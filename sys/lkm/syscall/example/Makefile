#	$NetBSD: Makefile,v 1.3 2001/12/12 11:56:06 lukem Exp $

.include "../Makefile.inc"

KMOD=	syscall_example

SRCS=	lkminit_syscall.c
SRCS+=	example_syscall.c

SUBDIR+= test

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
