#	$NetBSD: Makefile,v 1.9 1997/10/10 19:51:39 christos Exp $
#	@(#)Makefile	8.4 (Berkeley) 7/13/94

PROG=	mount_union
SRCS=	mount_union.c getmntopts.c
MAN=	mount_union.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
