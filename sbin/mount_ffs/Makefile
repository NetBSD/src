#	$NetBSD: Makefile,v 1.6 2003/03/22 12:43:59 jdolecek Exp $

.include <bsd.own.mk>

PROG=	mount_ffs
SRCS=	mount_ffs.c
MAN=	mount_ffs.8

MLINKS=	mount_ffs.8 mount_ufs.8
LINKS=  ${BINDIR}/mount_ffs ${BINDIR}/mount_ufs

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
