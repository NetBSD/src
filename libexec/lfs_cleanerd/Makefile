#	$NetBSD: Makefile,v 1.4 1997/10/07 13:44:02 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93

WARNS?=	1
PROG=	lfs_cleanerd
CFLAGS+=-I${.CURDIR} -DDIAGNOSTIC
MAN=	lfs_cleanerd.8
SRCS=	cleanerd.c lfs_cksum.c library.c misc.c print.c

.PATH:	${.CURDIR}/../../sys/ufs/lfs

.include <bsd.prog.mk>
