#	$NetBSD: Makefile,v 1.2 2008/08/05 20:57:45 pooka Exp $

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
