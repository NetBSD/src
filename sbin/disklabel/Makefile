#	$NetBSD: Makefile,v 1.16 1995/04/18 22:37:46 ragge Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/17/94

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c
MAN=	disklabel.5 disklabel.8

.if ${MACHINE} == "amiga"
CFLAGS+= -D${MACHINE}
.endif

.if (${MACHINE} == "i386") || (${MACHINE} == "alpha") || (${MACHINE} == "vax")
CFLAGS+= -DNUMBOOT=2
.endif

.if ${MACHINE} == "hp300"
CFLAGS+= -DNUMBOOT=1
.endif

.if ${MACHINE} == "i386"
CFLAGS+= -DRAWPARTITION=\'d\'
.endif

.include <bsd.prog.mk>
