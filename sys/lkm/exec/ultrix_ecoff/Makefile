#	$NetBSD: Makefile,v 1.5.2.2 2004/09/18 14:54:09 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

BUILDSYMLINKS+=${S}/arch/mips/include mips

.include <bsd.kmod.mk>
