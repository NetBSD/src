#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:05 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/hpux

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

KMOD=	exec_hpux_aout

SRCS=	lkminit_exec.c
SRCS+=	hpux_exec_aout.c

.include <bsd.kmod.mk>
