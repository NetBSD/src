#	$NetBSD: Makefile,v 1.4 1996/10/15 06:59:55 veego Exp $

PROG=	co
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} /usr/lib/libgnumalloc.a

.include "../../Makefile.inc"
.include <bsd.prog.mk>
