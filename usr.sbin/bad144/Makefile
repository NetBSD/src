#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.8 1999/01/15 13:32:06 bouyer Exp $

.if ${MACHINE} == "alpha" || ${MACHINE} == "i386"
PROG=	bad144
.endif
DPADD+= ${LIBUTIL}
LDADD+= -lutil

MAN=	bad144.8

.include <bsd.prog.mk>
