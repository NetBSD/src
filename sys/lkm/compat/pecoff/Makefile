#	$NetBSD: Makefile,v 1.5.14.1 2007/11/19 00:49:01 mjf Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-DCOMPAT_PECOFF -DSYSVMSG -DSYSVSHM

KMOD=	compat_pecoff

SRCS+=	lkminit_emul.c
SRCS+=	pecoff_exec.c
SRCS+=	pecoff_emul.c

.include <bsd.kmod.mk>
