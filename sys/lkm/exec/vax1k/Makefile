#	$NetBSD: Makefile,v 1.1.4.2 2002/01/08 00:33:17 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/vax1k

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

KMOD=	exec_vax1k

SRCS=	lkminit_exec.c
SRCS+=	vax1k_exec.c

.include <bsd.kmod.mk>
