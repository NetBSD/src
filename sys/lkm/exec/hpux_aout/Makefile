#	$NetBSD: Makefile,v 1.4 2002/05/30 18:53:17 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/hpux

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_hpux_aout

SRCS=	lkminit_exec.c
SRCS+=	hpux_exec_aout.c

.include <bsd.kmod.mk>
