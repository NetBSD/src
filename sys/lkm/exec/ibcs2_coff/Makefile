#	$NetBSD: Makefile,v 1.3 2001/12/12 11:49:05 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_COFF 

KMOD=	exec_ibcs2_coff

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_coff.c

.include <bsd.kmod.mk>
