#	$NetBSD: Makefile,v 1.11 2001/11/09 09:05:51 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	tunefs
MAN=	tunefs.8
SRCS=	tunefs.c ffs_bswap.c
LDADD+=-lutil

.PATH:  ${.CURDIR}/../../sys/ufs/ffs

WARNS?=2

.include <bsd.prog.mk>
