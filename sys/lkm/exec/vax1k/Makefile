#	$NetBSD: Makefile,v 1.1 2000/12/08 23:05:49 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/vax1k

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

MKMAN=	no

KMOD=	exec_vax1k

SRCS=	lkminit_exec.c
SRCS+=	vax1k_exec.c vax1k_subr.c

.include <bsd.kmod.mk>
