#	$NetBSD: Makefile,v 1.2 2001/11/26 20:38:15 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd
.PATH:	$S/compat/common

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_AOUT

MKMAN=	no

KMOD=	exec_freebsd_aout

SRCS=	lkminit_exec.c
SRCS+=	freebsd_exec_aout.c
SRCS+=	compat_exec.c

.include <bsd.kmod.mk>
