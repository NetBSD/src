#	$NetBSD: Makefile,v 1.6 2002/08/19 10:16:54 lukem Exp $

.include <bsd.own.mk>

PROG=	fsirand
SRCS=	fsirand.c ffs_bswap.c
MAN=	fsirand.8

LDADD+=	-lutil

WARNS=	2

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>
