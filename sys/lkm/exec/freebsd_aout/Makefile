#	$NetBSD: Makefile,v 1.5 2002/05/30 18:53:16 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd
.PATH:	$S/compat/common

CPPFLAGS+=	-nostdinc -D_KERNEL -DEXEC_AOUT

KMOD=	exec_freebsd_aout

SRCS=	lkminit_exec.c
SRCS+=	freebsd_exec_aout.c
SRCS+=	compat_exec.c

.include <bsd.kmod.mk>
