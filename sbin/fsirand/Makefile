#	$NetBSD: Makefile,v 1.5 2001/11/16 04:25:42 lukem Exp $

PROG=	fsirand
SRCS=	fsirand.c ffs_bswap.c
MAN=	fsirand.8

LDADD+=	-lutil

WARNS=	2

.PATH:  ${.CURDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
