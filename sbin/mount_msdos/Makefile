#	$NetBSD: Makefile,v 1.17 2002/09/21 18:43:35 christos Exp $

.include <bsd.own.mk>

PROG=	mount_msdos
SRCS=	mount_msdos.c getmntopts.c fattr.c
MAN=	mount_msdos.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
