#	$NetBSD: Makefile,v 1.28 2003/03/22 12:44:04 jdolecek Exp $
#	@(#)Makefile	8.6 (Berkeley) 5/8/95

.include <bsd.own.mk>

PROG=	mount
MAN=	mount.8
SRCS=	mount.c vfslist.c

.if defined(RESCUEDIR)
CPPFLAGS+= -D_PATH_RESCUE=\"${RESCUEDIR}\"
.endif

.include <bsd.prog.mk>
