#	from: @(#)Makefile	8.1 (Berkeley) 6/18/93
#	$Id: Makefile,v 1.1 1994/06/08 18:58:47 mycroft Exp $

PROG=	dumplfs
CFLAGS+=-I/sys/ufs/lfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
.PATH:	/sys/ufs/lfs
MAN8=	dumplfs.0

.include <bsd.prog.mk>
