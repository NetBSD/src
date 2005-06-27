#	$NetBSD: Makefile,v 1.13 2005/06/27 01:00:05 christos Exp $

.include <bsd.own.mk>

PROG=	mount_ados
SRCS=	mount_ados.c fattr.c
MAN=	mount_ados.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
