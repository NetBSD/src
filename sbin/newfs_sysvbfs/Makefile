# $NetBSD: Makefile,v 1.2.24.1 2008/09/18 04:28:29 wrstuden Exp $

.include <bsd.own.mk>

PROG=	newfs_sysvbfs
MAN=	newfs_sysvbfs.8

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS.newfs_sysvbfs.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
