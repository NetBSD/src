#	$NetBSD: Makefile,v 1.6 1997/10/10 19:50:12 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	mount_lfs
SRCS=	mount_lfs.c getmntopts.c
MAN=	mount_lfs.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
