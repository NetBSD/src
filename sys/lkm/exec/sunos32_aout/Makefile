#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:12 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/sunos32

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

NOMAN=	# defined

KMOD=	exec_sunos32_aout

SRCS=	lkminit_exec.c
SRCS+=	sunos32_exec_aout.c

.include <bsd.kmod.mk>
