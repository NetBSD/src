#	$NetBSD: Makefile,v 1.11 2003/03/22 12:43:57 jdolecek Exp $

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
