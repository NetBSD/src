#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:11 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/m68k4k

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

NOMAN=	# defined

KMOD=	exec_m68k4k

SRCS=	lkminit_exec.c
SRCS+=	m68k4k_exec.c

.include <bsd.kmod.mk>
