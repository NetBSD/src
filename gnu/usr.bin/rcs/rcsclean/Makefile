#	$NetBSD: Makefile,v 1.4 1996/10/15 07:00:36 veego Exp $

PROG=	rcsclean
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} /usr/lib/libgnumalloc.a

.include "../../Makefile.inc"
.include <bsd.prog.mk>
