#	$NetBSD: Makefile,v 1.3 2001/12/12 01:50:09 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd
.PATH:	$S/compat/common

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

NOMAN=	# defined

KMOD=	exec_freebsd_aout

SRCS=	lkminit_exec.c
SRCS+=	freebsd_exec_aout.c
SRCS+=	compat_exec.c

.include <bsd.kmod.mk>
