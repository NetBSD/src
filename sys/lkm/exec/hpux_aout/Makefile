#	$NetBSD: Makefile,v 1.3.8.1 2002/06/20 16:02:28 gehenna Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/hpux

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_hpux_aout

SRCS=	lkminit_exec.c
SRCS+=	hpux_exec_aout.c

.include <bsd.kmod.mk>
