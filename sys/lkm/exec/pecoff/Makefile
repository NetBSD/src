#	$NetBSD: Makefile,v 1.5 2007/05/12 10:45:01 dsl Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-nostdinc -D_KERNEL -DCOMPAT_PECOFF

MKMAN=	no

KMOD=	exec_pecoff

SRCS=	lkminit_exec.c
SRCS+=	pecoff_exec.c

.include <bsd.kmod.mk>
