#	$NetBSD: Makefile,v 1.3 2009/04/11 07:58:13 lukem Exp $

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
