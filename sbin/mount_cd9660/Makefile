#	$NetBSD: Makefile,v 1.8 2002/09/21 18:43:33 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_cd9660
SRCS=	mount_cd9660.c getmntopts.c
MAN=	mount_cd9660.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
