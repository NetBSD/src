#	$NetBSD: Makefile,v 1.2 2001/09/08 13:19:50 chuck Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/vax1k

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

MKMAN=	no

KMOD=	exec_vax1k

SRCS=	lkminit_exec.c
SRCS+=	vax1k_exec.c

.include <bsd.kmod.mk>
