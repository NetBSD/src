#	$NetBSD: Makefile,v 1.1.2.2 2001/01/08 14:57:18 bouyer Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32 -DEXEC_ELF64

MKMAN=	no

KMOD=	exec_ibcs2_xout

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_xout.c

.include <bsd.kmod.mk>
