#	from: @(#)Makefile	8.2 (Berkeley) 3/17/94
#	$Id: Makefile,v 1.12 1994/12/22 10:43:53 cgd Exp $

.include <bsd.own.mk>

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c
MAN=	disklabel.5 disklabel.8

.if ${MACHINE} == "amiga"
CFLAGS+= -D${MACHINE}
.endif

.if ${MACHINE} == "i386"
CFLAGS+= -DNUMBOOT=2
.endif

.if ${MACHINE} == "hp300"
CFLAGS+= -DNUMBOOT=1
.endif

.if ${MACHINE} == "i386"
CFLAGS+= -DRAWPARTITION=\'d\'
.endif

.include <bsd.prog.mk>
