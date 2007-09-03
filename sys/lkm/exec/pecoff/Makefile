#	$NetBSD: Makefile,v 1.4.22.1 2007/09/03 14:41:37 yamt Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-nostdinc -D_KERNEL -DCOMPAT_PECOFF

MKMAN=	no

KMOD=	exec_pecoff

SRCS=	lkminit_exec.c
SRCS+=	pecoff_exec.c

.include <bsd.kmod.mk>
