#	$NetBSD: Makefile,v 1.4 2005/12/11 12:24:47 christos Exp $

COMPAT_UTIL_OVERRIDE=	pecoff
.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-DCOMPAT_PECOFF -DSYSVMSG -DSYSVSHM

KMOD=	compat_pecoff

SRCS+=	lkminit_emul.c
SRCS+=	pecoff_sysent.c pecoff_syscalls.c pecoff_misc.c pecoff_emul.c

.include <bsd.kmod.mk>
