#	$NetBSD: Makefile,v 1.7 1997/05/06 20:45:23 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

PROG=	newlfs
SRCS=	dkcksum.c lfs.c lfs_cksum.c misc.c newfs.c
MAN=	newlfs.8
.PATH.c: ${.CURDIR}/../../sys/ufs/lfs ${.CURDIR}/../disklabel

DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
