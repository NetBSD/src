#	$NetBSD: Makefile,v 1.6 2003/08/01 17:04:02 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

DPSRCS+=	mips
CLEANFILES+=	mips
.PHONY: mips
mips: 
	-rm -f $@
	ln -s $S/arch/mips/include $@

.include <bsd.kmod.mk>
