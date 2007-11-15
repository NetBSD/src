#	$NetBSD: Makefile,v 1.3.16.2 2007/11/15 11:44:59 yamt Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-DCOMPAT_PECOFF -DSYSVMSG -DSYSVSHM

KMOD=	compat_pecoff

SRCS+=	lkminit_emul.c
SRCS+=	pecoff_exec.c
SRCS+=	pecoff_emul.c

.include <bsd.kmod.mk>
