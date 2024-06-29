#	$NetBSD: Makefile,v 1.1.2.1 2024/06/29 19:43:25 perseant Exp $

.include <bsd.own.mk>

PROG=	mount_exfatfs
SRCS=	mount_exfatfs.c fattr.c pathadj.c
MAN=	mount_exfatfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
