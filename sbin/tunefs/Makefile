#	$NetBSD: Makefile,v 1.12 2002/08/19 10:17:01 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	tunefs
MAN=	tunefs.8
SRCS=	tunefs.c ffs_bswap.c
LDADD+=-lutil

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

WARNS?=2

.include <bsd.prog.mk>
