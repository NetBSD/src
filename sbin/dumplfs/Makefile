#	from: @(#)Makefile	8.1 (Berkeley) 6/18/93
#	$Id: Makefile,v 1.2 1994/06/10 17:33:55 pk Exp $

PROG=	dumplfs
CFLAGS+=-I${DESTDIR}/sys/ufs/lfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
.PATH:	${DESTDIR}/sys/ufs/lfs
MAN8=	dumplfs.0

.include <bsd.prog.mk>
