#	$NetBSD: Makefile,v 1.12 1999/05/02 04:42:47 ross Exp $

SUBDIR=	mbr

.if (${MACHINE} == "i386" || ${MACHINE} == "powerpc")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
SUBDIR+= mbr_bootsel
.endif


MAN=	fdisk.8

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
