#	$NetBSD: Makefile,v 1.5 2003/05/04 06:36:37 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

DPSRCS+= mips
.PHONY: mips
mips: 
	-rm -f $@
	ln -s $S/arch/mips/include $@

.include <bsd.kmod.mk>
