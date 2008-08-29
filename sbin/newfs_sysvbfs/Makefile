# $NetBSD: Makefile,v 1.3 2008/08/29 00:02:23 gmcgarry Exp $

.include <bsd.own.mk>

PROG=	newfs_sysvbfs
MAN=	newfs_sysvbfs.8

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS.newfs_sysvbfs.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
