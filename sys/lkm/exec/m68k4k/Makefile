#	$NetBSD: Makefile,v 1.1 2000/12/08 23:05:42 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/m68k4k

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

MKMAN=	no

KMOD=	exec_m68k4k

SRCS=	lkminit_exec.c
SRCS+=	m68k4k_exec.c

.include <bsd.kmod.mk>
