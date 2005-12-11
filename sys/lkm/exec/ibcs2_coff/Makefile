#	$NetBSD: Makefile,v 1.6 2005/12/11 12:24:48 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_COFF

KMOD=	exec_ibcs2_coff

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_coff.c

.include <bsd.kmod.mk>
