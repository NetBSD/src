#	$NetBSD: Makefile,v 1.1.4.1 2002/01/08 00:33:10 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/m68k4k

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

KMOD=	exec_m68k4k

SRCS=	lkminit_exec.c
SRCS+=	m68k4k_exec.c

.include <bsd.kmod.mk>
