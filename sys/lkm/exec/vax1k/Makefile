#	$NetBSD: Makefile,v 1.1.8.1 2001/10/01 12:47:15 fvdl Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/vax1k

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

MKMAN=	no

KMOD=	exec_vax1k

SRCS=	lkminit_exec.c
SRCS+=	vax1k_exec.c

.include <bsd.kmod.mk>
