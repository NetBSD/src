#	$NetBSD: Makefile,v 1.16 2002/08/19 10:16:57 lukem Exp $

.include <bsd.own.mk>

PROG=	mount_msdos
SRCS=	mount_msdos.c getmntopts.c fattr.c
MAN=	mount_msdos.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys
.PATH:	${MOUNT}

.include <bsd.prog.mk>
