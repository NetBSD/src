#	$NetBSD: Makefile,v 1.1.6.2 2002/06/23 17:49:59 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/hpux

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_hpux_aout

SRCS=	lkminit_exec.c
SRCS+=	hpux_exec_aout.c

.include <bsd.kmod.mk>
