#	$NetBSD: Makefile,v 1.2.4.1 2009/05/13 19:19:03 jym Exp $

.include <bsd.own.mk>

PROG=	mount_udf
SRCS=	mount_udf.c fattr.c pathadj.c
MAN=	mount_udf.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
