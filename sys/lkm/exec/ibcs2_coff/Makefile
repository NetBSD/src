#	$NetBSD: Makefile,v 1.4.6.1 2005/03/04 16:52:47 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ibcs2

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_COFF

KMOD=	exec_ibcs2_coff

SRCS=	lkminit_exec.c
SRCS+=	ibcs2_exec_coff.c

.include <bsd.kmod.mk>
