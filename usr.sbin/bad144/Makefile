#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.9.40.1 2009/05/13 19:20:16 jym Exp $

.if ${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "i386"
PROG=	bad144
.endif
DPADD+= ${LIBUTIL}
LDADD+= -lutil

MAN=	bad144.8

.include <bsd.prog.mk>
