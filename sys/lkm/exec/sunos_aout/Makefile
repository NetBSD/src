#	$NetBSD: Makefile,v 1.3.8.1 2002/06/20 16:02:33 gehenna Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_sunos_aout

SRCS=	lkminit_exec.c
SRCS+=	sunos_exec_aout.c

.include <bsd.kmod.mk>
