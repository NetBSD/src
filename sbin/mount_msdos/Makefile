#	$NetBSD: Makefile,v 1.19 2005/02/05 15:02:20 xtraeme Exp $

.include <bsd.own.mk>

PROG=	mount_msdos
SRCS=	mount_msdos.c fattr.c
MAN=	mount_msdos.8

WARNS=	3

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
