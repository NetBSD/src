#	$NetBSD: Makefile,v 1.5.2.1 2004/08/03 10:53:58 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/ultrix

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_ECOFF

KMOD=	exec_ultrix_ecoff

SRCS=	lkminit_exec.c
SRCS+=	ultrix_exec_ecoff.c

BUILDSYMLINKS+=${S}/arch/mips/include mips

.include <bsd.kmod.mk>
