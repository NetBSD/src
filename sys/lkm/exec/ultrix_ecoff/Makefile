#	$NetBSD: Makefile,v 1.3 2001/12/12 11:49:08 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

DPSRCS+= mips
.PHONY: mips
mips: 
	ln -sf $S/arch/mips/include mips

.include <bsd.kmod.mk>
