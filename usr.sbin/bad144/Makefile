#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.9 2002/06/13 13:32:47 wiz Exp $

.if ${MACHINE} == "alpha" || ${MACHINE} == "i386"
PROG=	bad144
.endif
DPADD+= ${LIBUTIL}
LDADD+= -lutil
WARNS=	2

MAN=	bad144.8

.include <bsd.prog.mk>
