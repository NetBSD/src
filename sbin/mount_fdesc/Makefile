#	$NetBSD: Makefile,v 1.7 1997/05/06 20:45:09 gwr Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	mount_fdesc
SRCS=	mount_fdesc.c getmntopts.c
MAN=	mount_fdesc.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH.c: ${MOUNT}

.include <bsd.prog.mk>
