#	$NetBSD: Makefile,v 1.1.4.2 2002/06/20 03:47:44 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/hpux

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_hpux_aout

SRCS=	lkminit_exec.c
SRCS+=	hpux_exec_aout.c

.include <bsd.kmod.mk>
