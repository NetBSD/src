#	$NetBSD: Makefile,v 1.3 2001/12/12 11:49:04 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

KMOD=	exec_a.out

SRCS=	lkminit_exec.c
SRCS+=	exec_aout.c

.include <bsd.kmod.mk>
