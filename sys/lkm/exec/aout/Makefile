#	$NetBSD: Makefile,v 1.3.10.2 2002/04/01 18:48:10 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

KMOD=	exec_a.out

SRCS=	lkminit_exec.c
SRCS+=	exec_aout.c

.include <bsd.kmod.mk>
