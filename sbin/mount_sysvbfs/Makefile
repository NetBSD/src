#	$NetBSD: Makefile,v 1.1 2005/12/29 14:53:45 tsutsui Exp $

.include <bsd.own.mk>

PROG=	mount_sysvbfs
SRCS=	mount_sysvbfs.c fattr.c
MAN=	mount_sysvbfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:	${MOUNT}

CPPFLAGS+= -I{$NETBSDSRCDIR}/sys

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
