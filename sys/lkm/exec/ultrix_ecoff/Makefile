#	$NetBSD: Makefile,v 1.8 2003/09/20 06:20:43 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

BUILDSYMLINKS+=${S}/arch/mips/include mips

.include <bsd.kmod.mk>
