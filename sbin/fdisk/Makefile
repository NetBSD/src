#	$NetBSD: Makefile,v 1.7 1997/09/14 13:52:26 lukem Exp $


.if (${MACHINE} == "i386" || ${MACHINE} == "powerpc")
PROG=	fdisk 
SRCS=	fdisk.c

.if (${MACHINE} == "i386")
CFLAGS+= -DRAWPARTITION=\'d\'
.endif

.endif

MAN=	fdisk.8

WARNS=1

.include <bsd.prog.mk>
