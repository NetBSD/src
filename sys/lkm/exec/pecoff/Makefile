#	$NetBSD: Makefile,v 1.4.60.1 2007/05/17 13:41:49 yamt Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-nostdinc -D_KERNEL -DCOMPAT_PECOFF

MKMAN=	no

KMOD=	exec_pecoff

SRCS=	lkminit_exec.c
SRCS+=	pecoff_exec.c

.include <bsd.kmod.mk>
