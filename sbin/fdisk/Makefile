#	$NetBSD: Makefile,v 1.14 1999/09/25 04:18:20 enami Exp $

SUBDIR=	mbr
.if ${MACHINE} == "i386"
SUBDIR+= mbr_bootsel
.endif

.if (${MACHINE} == "i386" || ${MACHINE} == "bebox" || ${MACHINE} == "ofppc")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

MAN=	fdisk.8

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
