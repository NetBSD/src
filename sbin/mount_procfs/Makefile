#	$NetBSD: Makefile,v 1.9 1997/10/10 19:51:16 christos Exp $
#	@(#)Makefile	8.4 (Berkeley) 3/27/94

PROG=	mount_procfs
SRCS=	mount_procfs.c getmntopts.c
MAN=	mount_procfs.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
