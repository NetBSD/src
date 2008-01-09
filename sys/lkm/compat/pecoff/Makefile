#	$NetBSD: Makefile,v 1.5.8.1 2008/01/09 01:56:49 matt Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-DCOMPAT_PECOFF -DSYSVMSG -DSYSVSHM

KMOD=	compat_pecoff

SRCS+=	lkminit_emul.c
SRCS+=	pecoff_exec.c
SRCS+=	pecoff_emul.c

.include <bsd.kmod.mk>
