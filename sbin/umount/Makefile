#	$NetBSD: Makefile,v 1.11 1997/10/10 19:52:30 christos Exp $
#	@(#)Makefile	8.4 (Berkeley) 6/22/95

PROG=	umount
SRCS=	umount.c vfslist.c
MAN=	umount.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
