#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:20 nathanw Exp $

.include "../Makefile.inc"

KMOD=	syscall_example

SRCS=	lkminit_syscall.c
SRCS+=	example_syscall.c

SUBDIR+= test

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
