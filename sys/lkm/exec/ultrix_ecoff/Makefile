#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:16 nathanw Exp $

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
