#	$NetBSD: Makefile,v 1.4 2002/05/30 18:53:23 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_sunos_aout

SRCS=	lkminit_exec.c
SRCS+=	sunos_exec_aout.c

.include <bsd.kmod.mk>
