#	$NetBSD: Makefile,v 1.10 2002/08/19 10:16:59 lukem Exp $
#	@(#)Makefile	8.4 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_procfs
SRCS=	mount_procfs.c getmntopts.c
MAN=	mount_procfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
