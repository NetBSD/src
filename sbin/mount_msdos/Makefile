#	$NetBSD: Makefile,v 1.20.24.1 2008/09/18 04:28:27 wrstuden Exp $

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
