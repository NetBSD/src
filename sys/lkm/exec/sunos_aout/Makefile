#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:14 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

KMOD=	exec_sunos_aout

SRCS=	lkminit_exec.c
SRCS+=	sunos_exec_aout.c

.include <bsd.kmod.mk>
