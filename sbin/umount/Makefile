#	$NetBSD: Makefile,v 1.10 1997/09/16 12:24:05 lukem Exp $
#	@(#)Makefile	8.4 (Berkeley) 6/22/95

PROG=	umount
SRCS=	umount.c vfslist.c
MAN=	umount.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
