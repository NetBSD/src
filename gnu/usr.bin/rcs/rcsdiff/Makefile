#	$NetBSD: Makefile,v 1.1.1.2 1996/10/13 21:57:42 veego Exp $

PROG=	rcsdiff
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} /usr/lib/libgnumalloc.a

.include "../../Makefile.inc"
.include <bsd.prog.mk>
