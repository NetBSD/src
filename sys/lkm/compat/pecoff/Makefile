#	$NetBSD: Makefile,v 1.2.8.2 2002/06/23 17:49:56 jdolecek Exp $

COMPAT_UTIL_OVERRIDE=	pecoff
.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-DCOMPAT_PECOFF

KMOD=	compat_pecoff

SRCS+=	lkminit_emul.c
SRCS+=	pecoff_sysent.c pecoff_syscalls.c pecoff_misc.c pecoff_emul.c

.include <bsd.kmod.mk>
