#	$NetBSD: Makefile,v 1.3.8.1 2002/06/20 16:02:32 gehenna Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos32

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_sunos32_aout

SRCS=	lkminit_exec.c
SRCS+=	sunos32_exec_aout.c

.include <bsd.kmod.mk>
