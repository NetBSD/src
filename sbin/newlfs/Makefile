#	from: @(#)Makefile	8.1 (Berkeley) 6/18/93
#	$Id: Makefile,v 1.2 1994/06/10 17:36:07 pk Exp $

PROG=	newlfs
CFLAGS+=-I${DESTDIR}/sys/ufs/lfs
SRCS=	dkcksum.c lfs.c lfs_cksum.c misc.c newfs.c
MAN8=	newlfs.0
.PATH:	${DESTDIR}/sys/ufs/lfs ${.CURDIR}/../disklabel

.include <bsd.prog.mk>
