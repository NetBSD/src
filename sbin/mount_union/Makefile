#	$NetBSD: Makefile,v 1.8 1997/09/16 12:32:52 lukem Exp $
#	@(#)Makefile	8.4 (Berkeley) 7/13/94

PROG=	mount_union
SRCS=	mount_union.c getmntopts.c
MAN=	mount_union.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
