#	$NetBSD: Makefile,v 1.8 1997/09/15 12:45:51 lukem Exp $


.if (${MACHINE} == "i386" || ${MACHINE} == "powerpc")
PROG=	fdisk 
SRCS=	fdisk.c

.if (${MACHINE} == "i386")
CFLAGS+= -DRAWPARTITION=\'d\'
.endif

.endif

MAN=	fdisk.8

.include <bsd.prog.mk>
