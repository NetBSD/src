#	$NetBSD: Makefile,v 1.8 2002/06/06 00:56:49 perseant Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	lfs_cleanerd
CPPFLAGS+=-I${.CURDIR} -DDIAGNOSTIC
MAN=	lfs_cleanerd.8
SRCS=	cleanerd.c lfs_cksum.c library.c misc.c print.c coalesce.c
LDADD+=-lutil
DPADD+=${LIBUTIL}

.PATH:	${.CURDIR}/../../sys/ufs/lfs

.include <bsd.prog.mk>
