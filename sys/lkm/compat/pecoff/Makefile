#	$NetBSD: Makefile,v 1.1 2002/03/25 06:52:43 kent Exp $

#COMPAT_UTIL_OVERRIDE=	pecoff
.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-DCOMPAT_PECOFF

KMOD=	compat_pecoff

SRCS+=	lkminit_emul.c
SRCS+=	pecoff_sysent.c pecoff_syscalls.c pecoff_misc.c pecoff_emul.c

.include <bsd.kmod.mk>
