#	$NetBSD: Makefile,v 1.7 1997/05/08 21:11:24 gwr Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_null
SRCS=	mount_null.c getmntopts.c
MAN=	mount_null.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
