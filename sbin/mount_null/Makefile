#	$NetBSD: Makefile,v 1.6 1997/05/06 20:45:15 gwr Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_null
SRCS=	mount_null.c getmntopts.c
MAN=	mount_null.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH.c: ${MOUNT}

.include <bsd.prog.mk>
