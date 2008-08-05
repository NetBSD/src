#	$NetBSD: Makefile,v 1.21 2008/08/05 20:57:45 pooka Exp $

.include <bsd.own.mk>

PROG=	mount_msdos
SRCS=	mount_msdos.c fattr.c pathadj.c
MAN=	mount_msdos.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
