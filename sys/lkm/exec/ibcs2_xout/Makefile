#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:10 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32 -DEXEC_ELF64

NOMAN=	# defined

KMOD=	exec_ibcs2_xout

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_xout.c

.include <bsd.kmod.mk>
