#	$NetBSD: Makefile,v 1.4 1997/05/06 20:45:08 gwr Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_cd9660
SRCS=	mount_cd9660.c getmntopts.c
MAN=	mount_cd9660.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH.c: ${MOUNT}

.include <bsd.prog.mk>
