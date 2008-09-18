#	$NetBSD: Makefile,v 1.2.16.1 2008/09/18 04:28:27 wrstuden Exp $

.include <bsd.own.mk>

PROG=	mount_hfs
SRCS=	mount_hfs.c pathadj.c
MAN=	mount_hfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS=-I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
