#	$NetBSD: Makefile,v 1.3.2.2 2002/01/10 20:00:56 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/kern

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

KMOD=	exec_a.out

SRCS=	lkminit_exec.c
SRCS+=	exec_aout.c

.include <bsd.kmod.mk>
