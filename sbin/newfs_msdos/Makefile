# $FreeBSD: src/sbin/newfs_msdos/Makefile,v 1.5 2001/03/26 14:33:18 ru Exp $

PROG=	newfs_msdos
MAN=	newfs_msdos.8

.if ${MACHINE} == "pc98"
CFLAGS+= -DPC98
.endif

.include <bsd.prog.mk>
