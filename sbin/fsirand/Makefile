#	$NetBSD: Makefile,v 1.9 2016/01/23 21:22:47 christos Exp $

.include <bsd.own.mk>

PROG=	fsirand
SRCS=	fsirand.c ffs_bswap.c
MAN=	fsirand.8

LDADD+=	-lutil
CPPFLAGS+= -D_KERNTYPES

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>
