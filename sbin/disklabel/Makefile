#	$NetBSD: Makefile,v 1.24 1997/06/07 00:47:27 perry Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/17/94

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c interact.c
MAN=	disklabel.5 disklabel.8
LDADD+= -lutil
DPADD+= ${LIBUTIL}

# XXX new boot blocks don't need this; we'll leave it for a while. --pm
#.if (${MACHINE} == "i386")
#CFLAGS+= -DNUMBOOT=2
#.endif

.if (${MACHINE} == "hp300") || (${MACHINE} == "vax")
CFLAGS+= -DNUMBOOT=1
.endif

.if ${MACHINE} == "i386"
CFLAGS+= -DRAWPARTITION=\'d\'
.endif

.include <bsd.prog.mk>
