#	$NetBSD: Makefile,v 1.1.12.1 2008/09/28 11:17:12 mjf Exp $

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
