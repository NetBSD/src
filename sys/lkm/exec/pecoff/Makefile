#	$NetBSD: Makefile,v 1.2.2.2 2002/04/01 18:48:11 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DCOMPAT_PECOFF

MKMAN=	no

KMOD=	exec_pecoff

SRCS=	lkminit_exec.c
SRCS+=	pecoff_exec.c

.include <bsd.kmod.mk>
