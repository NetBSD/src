#	$NetBSD: Makefile,v 1.5 2002/05/30 18:53:26 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/vax1k

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_vax1k

SRCS=	lkminit_exec.c
SRCS+=	vax1k_exec.c

.include <bsd.kmod.mk>
