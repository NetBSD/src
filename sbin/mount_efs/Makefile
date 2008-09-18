#	$NetBSD: Makefile,v 1.1.16.1 2008/09/18 04:28:26 wrstuden Exp $

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
