#	$NetBSD: Makefile,v 1.1.6.2 2002/06/23 17:50:01 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ELF32 -DEXEC_ELF64

KMOD=	exec_ibcs2_xout

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_xout.c

.include <bsd.kmod.mk>
