#	$NetBSD: Makefile,v 1.9 1997/10/10 19:49:31 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	mount_fdesc
SRCS=	mount_fdesc.c getmntopts.c
MAN=	mount_fdesc.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
