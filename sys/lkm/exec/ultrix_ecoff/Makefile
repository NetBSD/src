#	$NetBSD: Makefile,v 1.9 2005/12/11 12:24:49 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

BUILDSYMLINKS+=${S}/arch/mips/include mips

.include <bsd.kmod.mk>
