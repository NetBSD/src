#	$NetBSD: Makefile,v 1.3 1997/06/23 14:48:03 mrg Exp $

.if ${MACHINE} == "i386"
SRCS=	apmd.c apmsubr.c
#LDADD+=	-lutil
#DPADD+=	${LIBUTIL}

CFLAGS+=	-g  -O2 -Wmissing-prototypes -Wall
PROG=	apmd
.endif

MAN=	apmd.8
MANSUBDIR=/i386

.include <bsd.prog.mk>
# DO NOT DELETE THIS LINE -- make depend depends on it.
