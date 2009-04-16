#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.11 2009/04/16 06:27:43 lukem Exp $

.if ${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "i386"
PROG=	bad144
.endif
DPADD+= ${LIBUTIL}
LDADD+= -lutil

MAN=	bad144.8

.include <bsd.prog.mk>
