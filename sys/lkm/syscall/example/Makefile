#	$NetBSD: Makefile,v 1.1.6.1 2002/01/10 20:01:15 thorpej Exp $

.include "../Makefile.inc"

KMOD=	syscall_example

SRCS=	lkminit_syscall.c
SRCS+=	example_syscall.c

SUBDIR+= test

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
