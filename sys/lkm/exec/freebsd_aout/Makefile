#	$NetBSD: Makefile,v 1.1.4.2 2002/06/20 03:47:43 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd
.PATH:	$S/compat/common

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_freebsd_aout

SRCS=	lkminit_exec.c
SRCS+=	freebsd_exec_aout.c
SRCS+=	compat_exec.c

.include <bsd.kmod.mk>
