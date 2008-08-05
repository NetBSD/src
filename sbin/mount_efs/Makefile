#	$NetBSD: Makefile,v 1.2 2008/08/05 20:57:45 pooka Exp $

.include <bsd.own.mk>

PROG=	mount_efs
SRCS=	mount_efs.c pathadj.c
MAN=	mount_efs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+=-I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
