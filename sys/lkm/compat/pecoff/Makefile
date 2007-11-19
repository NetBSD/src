#	$NetBSD: Makefile,v 1.5 2007/04/22 08:30:01 dsl Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-DCOMPAT_PECOFF -DSYSVMSG -DSYSVSHM

KMOD=	compat_pecoff

SRCS+=	lkminit_emul.c
SRCS+=	pecoff_sysent.c pecoff_syscalls.c pecoff_misc.c pecoff_emul.c

.include <bsd.kmod.mk>
