#	$NetBSD: Makefile,v 1.1.1.2 1996/10/13 21:57:26 veego Exp $

PROG=	rcs
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} /usr/lib/libgnumalloc.a

MAN=	rcs.1 rcsfile.5 rcsintro.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
