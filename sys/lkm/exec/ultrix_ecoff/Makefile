#	$NetBSD: Makefile,v 1.7 2003/09/04 07:18:02 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

LNFILES+=${S}/arch/mips/include mips

.include <bsd.kmod.mk>
