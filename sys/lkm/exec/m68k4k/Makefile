#	$NetBSD: Makefile,v 1.1.4.2 2002/06/20 03:47:48 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/m68k4k

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_m68k4k

SRCS=	lkminit_exec.c
SRCS+=	m68k4k_exec.c

.include <bsd.kmod.mk>
