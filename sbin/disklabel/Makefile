#	from: @(#)Makefile	8.2 (Berkeley) 3/17/94
#	$Id: Makefile,v 1.14 1995/02/11 02:42:21 cgd Exp $

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c
MAN=	disklabel.5 disklabel.8

.if ${MACHINE} == "amiga"
CFLAGS+= -D${MACHINE}
.endif

.if (${MACHINE} == "i386") || (${MACHINE} == "alpha")
CFLAGS+= -DNUMBOOT=2
.endif

.if ${MACHINE} == "hp300"
CFLAGS+= -DNUMBOOT=1
.endif

.if ${MACHINE} == "i386"
CFLAGS+= -DRAWPARTITION=\'d\'
.endif

.include <bsd.prog.mk>
