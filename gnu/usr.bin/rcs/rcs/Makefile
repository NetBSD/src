#	$NetBSD: Makefile,v 1.5 1996/10/15 07:00:32 veego Exp $

PROG=	rcs
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} /usr/lib/libgnumalloc.a

MAN=	rcs.1 rcsfile.5 rcsintro.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
