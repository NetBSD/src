#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:15 tv Exp $

.include "../Makefile.inc"

KMOD=	syscall_example

NOMAN=	# defined

SRCS=	lkminit_syscall.c
SRCS+=	example_syscall.c

SUBDIR+= test

.include <bsd.kmod.mk>
.include <bsd.subdir.mk>
