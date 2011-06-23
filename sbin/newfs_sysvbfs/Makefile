# $NetBSD: Makefile,v 1.3.16.1 2011/06/23 14:18:44 cherry Exp $

.include <bsd.own.mk>

PROG=	newfs_sysvbfs
MAN=	newfs_sysvbfs.8

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.if defined(HAVE_GCC)
COPTS.newfs_sysvbfs.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
