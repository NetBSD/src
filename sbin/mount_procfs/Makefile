#	$NetBSD: Makefile,v 1.11 2002/09/21 18:43:38 christos Exp $
#	@(#)Makefile	8.4 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_procfs
SRCS=	mount_procfs.c getmntopts.c
MAN=	mount_procfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
