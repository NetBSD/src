#	$NetBSD: Makefile,v 1.1.6.1 2002/01/10 20:01:08 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

KMOD=	exec_sunos_aout

SRCS=	lkminit_exec.c
SRCS+=	sunos_exec_aout.c

.include <bsd.kmod.mk>
