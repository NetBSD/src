#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.7 1997/10/17 05:31:02 lukem Exp $

.if ${MACHINE} == "alpha" || ${MACHINE} == "i386"
PROG=	bad144
.endif
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

MAN=	bad144.8

.include <bsd.prog.mk>
