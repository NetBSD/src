#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.10 2009/02/14 13:56:41 abs Exp $

.if ${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "i386"
PROG=	bad144
.endif
DPADD+= ${LIBUTIL}
LDADD+= -lutil
WARNS=	2

MAN=	bad144.8

.include <bsd.prog.mk>
