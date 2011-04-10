#	$NetBSD: Makefile,v 1.4 2011/04/10 16:47:05 joerg Exp $

.include <bsd.own.mk>

PROG=	mount_hfs
SRCS=	mount_hfs.c pathadj.c
MAN=	mount_hfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+=-I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
