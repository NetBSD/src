#	$NetBSD: Makefile,v 1.8 1997/10/10 19:50:52 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_null
SRCS=	mount_null.c getmntopts.c
MAN=	mount_null.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
