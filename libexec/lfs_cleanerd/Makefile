#	$NetBSD: Makefile,v 1.6 1997/10/22 05:51:29 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	lfs_cleanerd
CPPFLAGS+=-I${.CURDIR} -DDIAGNOSTIC
MAN=	lfs_cleanerd.8
SRCS=	cleanerd.c lfs_cksum.c library.c misc.c print.c

.PATH:	${.CURDIR}/../../sys/ufs/lfs

.include <bsd.prog.mk>
