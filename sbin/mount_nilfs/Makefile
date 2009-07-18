#	$NetBSD: Makefile,v 1.1 2009/07/18 16:31:42 reinoud Exp $

.include <bsd.own.mk>

WARNS=	4
PROG=	mount_nilfs
SRCS=	mount_nilfs.c fattr.c pathadj.c
MAN=	mount_nilfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
