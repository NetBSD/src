#	$NetBSD: Makefile,v 1.8 1997/05/08 21:11:30 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

PROG=	newlfs
SRCS=	dkcksum.c lfs.c lfs_cksum.c misc.c newfs.c
MAN=	newlfs.8
.PATH:	${.CURDIR}/../../sys/ufs/lfs ${.CURDIR}/../disklabel

DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
