#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93
#	$Id: Makefile,v 1.2 1994/08/03 20:46:47 deraadt Exp $

PROG=	lfs_cleanerd
CFLAGS+=-I${.CURDIR} -DDIAGNOSTIC
MAN8=	lfs_cleanerd.0
SRCS=	cleanerd.c lfs_cksum.c library.c misc.c print.c

.PATH:	${.CURDIR}/../../sys/ufs/lfs

.include <bsd.prog.mk>
