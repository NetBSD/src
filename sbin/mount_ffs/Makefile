#	$NetBSD: Makefile,v 1.5 2002/08/19 10:16:56 lukem Exp $

.include <bsd.own.mk>

PROG=	mount_ffs
SRCS=	mount_ffs.c getmntopts.c
MAN=	mount_ffs.8

MLINKS=	mount_ffs.8 mount_ufs.8
LINKS=  ${BINDIR}/mount_ffs ${BINDIR}/mount_ufs

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
