#	$NetBSD: Makefile,v 1.13 1999/07/02 17:48:26 sakamoto Exp $

SUBDIR=	mbr

.if (${MACHINE} == "i386" || ${MACHINE} == "bebox" || ${MACHINE} == "ofppc")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
SUBDIR+= mbr_bootsel
.endif


MAN=	fdisk.8

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
