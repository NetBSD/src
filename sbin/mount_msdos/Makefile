#	$NetBSD: Makefile,v 1.18 2003/03/22 12:44:00 jdolecek Exp $

.include <bsd.own.mk>

PROG=	mount_msdos
SRCS=	mount_msdos.c fattr.c
MAN=	mount_msdos.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
