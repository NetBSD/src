#	$NetBSD: Makefile,v 1.3 2001/12/12 01:50:14 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/vax1k

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

NOMAN=	# defined

KMOD=	exec_vax1k

SRCS=	lkminit_exec.c
SRCS+=	vax1k_exec.c

.include <bsd.kmod.mk>
