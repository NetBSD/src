#	from: @(#)Makefile	8.1 (Berkeley) 6/18/93
#	$Id: Makefile,v 1.3 1994/08/03 21:07:21 deraadt Exp $

PROG=	newlfs
SRCS=	dkcksum.c lfs.c lfs_cksum.c misc.c newfs.c
MAN8=	newlfs.0
.PATH:	${.CURDIR}/../../sys/ufs/lfs ${.CURDIR}/../disklabel

.include <bsd.prog.mk>
