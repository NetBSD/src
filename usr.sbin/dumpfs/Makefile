#	$NetBSD: Makefile,v 1.11 2001/11/09 12:01:13 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	dumpfs
MAN=	dumpfs.8
SRCS=	dumpfs.c ffs_bswap.c
LDADD+=	-lutil

WARNS=	2

.PATH:  ${.CURDIR}/../../sys/ufs/ffs


.include <bsd.prog.mk>
