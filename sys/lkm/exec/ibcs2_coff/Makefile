#	$NetBSD: Makefile,v 1.1 2000/12/08 23:05:37 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_COFF 

MKMAN=	no

KMOD=	exec_ibcs2_coff

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_coff.c

.include <bsd.kmod.mk>
