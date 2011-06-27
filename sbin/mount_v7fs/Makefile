#	$NetBSD: Makefile,v 1.1 2011/06/27 11:52:58 uch Exp $

.include <bsd.own.mk>

PROG=	mount_v7fs
SRCS=	mount_v7fs.c fattr.c pathadj.c
MAN=	mount_v7fs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
