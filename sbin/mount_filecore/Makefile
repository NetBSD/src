#	$NetBSD: Makefile,v 1.7 2005/02/05 14:55:44 xtraeme Exp $
#
#	Makefile	1.0	1998/6/26

.include <bsd.own.mk>

PROG=	mount_filecore
SRCS=	mount_filecore.c fattr.c
MAN=	mount_filecore.8

WARNS=	3

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH: ${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
