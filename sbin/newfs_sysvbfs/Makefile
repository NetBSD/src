# $NetBSD: Makefile,v 1.4 2011/06/20 07:44:00 mrg Exp $

.include <bsd.own.mk>

PROG=	newfs_sysvbfs
MAN=	newfs_sysvbfs.8

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.if defined(HAVE_GCC)
COPTS.newfs_sysvbfs.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
