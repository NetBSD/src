#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:13 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ECOFF

NOMAN=	# defined

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

DPSRCS+= mips
.PHONY: mips
mips: 
	ln -sf $S/arch/mips/include mips

.include <bsd.kmod.mk>
