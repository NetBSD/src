#	$NetBSD: Makefile,v 1.4.8.1 2002/06/20 16:02:28 gehenna Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd
.PATH:	$S/compat/common

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_freebsd_aout

SRCS=	lkminit_exec.c
SRCS+=	freebsd_exec_aout.c
SRCS+=	compat_exec.c

.include <bsd.kmod.mk>
