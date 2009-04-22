#	$NetBSD: Makefile,v 1.15 2009/04/22 15:23:02 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	dumpfs
MAN=	dumpfs.8
SRCS=	dumpfs.c ffs_bswap.c

.ifndef HOSTPROG
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
.endif

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs


.include <bsd.prog.mk>
