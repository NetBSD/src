#	$NetBSD: Makefile,v 1.15 2002/08/23 03:19:07 lukem Exp $

PROG=	fsck
SRCS=	fsck.c fsutil.c preen.c
MAN=	fsck.8

.if defined(RESCUEDIR)
CPPFLAGS+= -D_PATH_RESCUE=\"${RESCUEDIR}\"
.endif

.include <bsd.prog.mk>
