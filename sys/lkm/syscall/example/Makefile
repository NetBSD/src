#	$NetBSD: Makefile,v 1.1 2000/12/11 13:48:12 jdolecek Exp $

.include "../Makefile.inc"

KMOD=	syscall_example

MKMAN=	no

SRCS=	lkminit_syscall.c
SRCS+=	example_syscall.c

SUBDIR+= test

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
