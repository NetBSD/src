#	$NetBSD: Makefile,v 1.4 2002/05/30 18:53:19 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/m68k4k

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_m68k4k

SRCS=	lkminit_exec.c
SRCS+=	m68k4k_exec.c

.include <bsd.kmod.mk>
