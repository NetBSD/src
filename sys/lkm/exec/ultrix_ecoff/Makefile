#	$NetBSD: Makefile,v 1.1.2.2 2001/01/08 14:57:28 bouyer Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ECOFF

MKMAN=	no

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

DPSRCS+= mips
.PHONY: mips
mips: 
	ln -sf $S/arch/mips/include mips

.include <bsd.kmod.mk>
