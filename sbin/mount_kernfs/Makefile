#	$NetBSD: Makefile,v 1.10 2002/08/19 10:16:57 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_kernfs
SRCS=	mount_kernfs.c getmntopts.c
MAN=	mount_kernfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
