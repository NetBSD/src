#	$NetBSD: Makefile,v 1.3.10.3 2002/06/20 03:47:42 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_a.out

SRCS=	lkminit_exec.c
SRCS+=	exec_aout.c

.include <bsd.kmod.mk>
