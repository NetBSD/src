#	$NetBSD: Makefile,v 1.1.4.2 2002/06/20 03:47:51 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_sunos_aout

SRCS=	lkminit_exec.c
SRCS+=	sunos_exec_aout.c

.include <bsd.kmod.mk>
