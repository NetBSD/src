#	$NetBSD: Makefile,v 1.1.4.2 2002/06/20 03:47:45 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_COFF 

KMOD=	exec_ibcs2_coff

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_coff.c

.include <bsd.kmod.mk>
