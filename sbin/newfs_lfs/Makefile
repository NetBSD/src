#	$NetBSD: Makefile,v 1.1 1999/03/18 17:18:05 perseant Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

PROG=	newfs_lfs
SRCS=	dkcksum.c lfs.c lfs_cksum.c misc.c newfs.c
MAN=	newfs_lfs.8
.PATH:	${.CURDIR}/../../sys/ufs/lfs ${.CURDIR}/../disklabel

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.include <bsd.prog.mk>
