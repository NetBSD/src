#	$NetBSD: Makefile,v 1.1 2006/02/02 15:21:29 reinoud Exp $

.include <bsd.own.mk>

WARNS=	4
PROG=	mount_udf
SRCS=	mount_udf.c fattr.c
MAN=	mount_udf.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
