#	from: @(#)Makefile	8.1 (Berkeley) 6/18/93
#	$Id: Makefile,v 1.1 1994/06/08 19:31:16 mycroft Exp $

PROG=	newlfs
CFLAGS+=-I/sys/ufs/lfs
SRCS=	dkcksum.c lfs.c lfs_cksum.c misc.c newfs.c
MAN8=	newlfs.0
.PATH:	/sys/ufs/lfs ${.CURDIR}/../disklabel

.include <bsd.prog.mk>
