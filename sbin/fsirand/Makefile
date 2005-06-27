#	$NetBSD: Makefile,v 1.8 2005/06/27 01:00:05 christos Exp $

.include <bsd.own.mk>

PROG=	fsirand
SRCS=	fsirand.c ffs_bswap.c
MAN=	fsirand.8

LDADD+=	-lutil

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>
