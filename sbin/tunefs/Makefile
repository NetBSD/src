#	$NetBSD: Makefile,v 1.14 2005/06/27 01:00:07 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	tunefs
MAN=	tunefs.8
SRCS=	tunefs.c ffs_bswap.c
LDADD+=-lutil

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>
