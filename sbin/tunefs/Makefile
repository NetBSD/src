#	$NetBSD: Makefile,v 1.13 2005/02/09 06:26:13 xtraeme Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	tunefs
MAN=	tunefs.8
SRCS=	tunefs.c ffs_bswap.c
LDADD+=-lutil

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

WARNS=	3

.include <bsd.prog.mk>
