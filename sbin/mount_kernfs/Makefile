#	$NetBSD: Makefile,v 1.8 1997/05/08 21:11:18 gwr Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	mount_kernfs
SRCS=	mount_kernfs.c getmntopts.c
MAN=	mount_kernfs.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
