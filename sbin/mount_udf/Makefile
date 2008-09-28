#	$NetBSD: Makefile,v 1.1.20.1 2008/09/28 11:17:14 mjf Exp $

.include <bsd.own.mk>

WARNS=	4
PROG=	mount_udf
SRCS=	mount_udf.c fattr.c pathadj.c
MAN=	mount_udf.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
