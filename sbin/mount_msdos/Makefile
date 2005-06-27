#	$NetBSD: Makefile,v 1.20 2005/06/27 01:00:06 christos Exp $

.include <bsd.own.mk>

PROG=	mount_msdos
SRCS=	mount_msdos.c fattr.c
MAN=	mount_msdos.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
