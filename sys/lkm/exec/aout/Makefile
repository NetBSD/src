#	$NetBSD: Makefile,v 1.3.12.1 2002/06/20 16:02:27 gehenna Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_a.out

SRCS=	lkminit_exec.c
SRCS+=	exec_aout.c

.include <bsd.kmod.mk>
