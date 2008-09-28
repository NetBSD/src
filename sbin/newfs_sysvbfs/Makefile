# $NetBSD: Makefile,v 1.2.20.1 2008/09/28 11:17:14 mjf Exp $

.include <bsd.own.mk>

PROG=	newfs_sysvbfs
MAN=	newfs_sysvbfs.8

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS.newfs_sysvbfs.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
