#	$NetBSD: Makefile,v 1.29 1997/10/18 07:28:47 mark Exp $
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

.if (${MACHINE} == "hp300") || (${MACHINE} == "vax") \
	|| (${MACHINE} == "arm32")
CPPFLAGS+= -DNUMBOOT=1
.endif

.include <bsd.prog.mk>
