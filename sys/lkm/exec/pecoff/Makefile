#	$NetBSD: Makefile,v 1.6 2008/03/10 01:22:05 dholland Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-nostdinc -D_KERNEL -DCOMPAT_PECOFF

KMOD=	exec_pecoff

SRCS=	lkminit_exec.c
SRCS+=	pecoff_exec.c

.include <bsd.kmod.mk>
