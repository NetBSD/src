#	$NetBSD: Makefile,v 1.7 2005/01/20 16:02:32 xtraeme Exp $

.include <bsd.own.mk>

PROG=	fsirand
SRCS=	fsirand.c ffs_bswap.c
MAN=	fsirand.8

LDADD+=	-lutil

WARNS=	3

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>
