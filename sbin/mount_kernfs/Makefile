#	$NetBSD: Makefile,v 1.9 1997/10/10 19:49:57 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	mount_kernfs
SRCS=	mount_kernfs.c getmntopts.c
MAN=	mount_kernfs.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
