#	$NetBSD: Makefile,v 1.14 2004/01/03 19:32:58 dbj Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	dumpfs
MAN=	dumpfs.8
SRCS=	dumpfs.c ffs_bswap.c

.ifndef HOSTPROG
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
.endif

WARNS=	2

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs


.include <bsd.prog.mk>
