#	$NetBSD: Makefile,v 1.7 2005/02/05 14:54:35 xtraeme Exp $

.include <bsd.own.mk>

PROG=	mount_ffs
SRCS=	mount_ffs.c
MAN=	mount_ffs.8

WARNS=	3

MLINKS=	mount_ffs.8 mount_ufs.8
LINKS=  ${BINDIR}/mount_ffs ${BINDIR}/mount_ufs

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
