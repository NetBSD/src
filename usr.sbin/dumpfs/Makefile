#	$NetBSD: Makefile,v 1.13 2003/12/28 06:04:23 dbj Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	dumpfs
MAN=	dumpfs.8
SRCS=	dumpfs.c ffs_bswap.c
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

WARNS=	2

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs


.include <bsd.prog.mk>
