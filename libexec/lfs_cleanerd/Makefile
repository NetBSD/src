#	$NetBSD: Makefile,v 1.6.10.1 2001/02/03 21:42:27 he Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	lfs_cleanerd
CPPFLAGS+=-I${.CURDIR} -DDIAGNOSTIC
MAN=	lfs_cleanerd.8
SRCS=	cleanerd.c lfs_cksum.c library.c misc.c print.c
LDADD+=-lutil
DPADD+=${LIBUTIL}

.PATH:	${.CURDIR}/../../sys/ufs/lfs

.include <bsd.prog.mk>
