#	$NetBSD: Makefile,v 1.4 1997/05/06 20:45:12 gwr Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	mount_lfs
SRCS=	mount_lfs.c getmntopts.c
MAN=	mount_lfs.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH.c: ${MOUNT}

.include <bsd.prog.mk>
