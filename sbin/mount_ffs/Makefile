#	$NetBSD: Makefile,v 1.10 2011/04/10 16:47:05 joerg Exp $

.include <bsd.own.mk>

PROG=	mount_ffs
SRCS=	mount_ffs.c pathadj.c
MAN=	mount_ffs.8

MLINKS=	mount_ffs.8 mount_ufs.8
LINKS=  ${BINDIR}/mount_ffs ${BINDIR}/mount_ufs

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+=-I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
