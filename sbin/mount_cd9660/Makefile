#	$NetBSD: Makefile,v 1.6 1997/10/10 19:49:10 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_cd9660
SRCS=	mount_cd9660.c getmntopts.c
MAN=	mount_cd9660.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
