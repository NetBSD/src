#	$NetBSD: Makefile,v 1.28 1997/10/10 19:47:50 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/17/94

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c interact.c
MAN=	disklabel.5 disklabel.8
LDADD+= -lutil
DPADD+= ${LIBUTIL}

# XXX new boot blocks don't need this; we'll leave it for a while. --pm
#.if (${MACHINE} == "i386")
#CPPFLAGS+= -DNUMBOOT=2
#.endif

.if (${MACHINE} == "hp300") || (${MACHINE} == "vax")
CPPFLAGS+= -DNUMBOOT=1
.endif

.include <bsd.prog.mk>
