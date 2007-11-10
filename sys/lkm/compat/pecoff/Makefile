#	$NetBSD: Makefile,v 1.6 2007/11/10 09:43:13 dsl Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-DCOMPAT_PECOFF -DSYSVMSG -DSYSVSHM

KMOD=	compat_pecoff

SRCS+=	lkminit_emul.c
SRCS+=	pecoff_exec.c
SRCS+=	pecoff_emul.c

.include <bsd.kmod.mk>
