#	$NetBSD: Makefile,v 1.10 2002/08/19 10:16:55 lukem Exp $

.include <bsd.own.mk>

PROG=	mount_ados
SRCS=	mount_ados.c getmntopts.c fattr.c
MAN=	mount_ados.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
