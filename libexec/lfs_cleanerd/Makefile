#	$NetBSD: Makefile,v 1.9 2002/08/19 13:54:36 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	lfs_cleanerd
CPPFLAGS+=-I${.CURDIR} -DDIAGNOSTIC
MAN=	lfs_cleanerd.8
SRCS=	cleanerd.c lfs_cksum.c library.c misc.c print.c coalesce.c
LDADD+=-lutil
DPADD+=${LIBUTIL}

.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs

.include <bsd.prog.mk>
