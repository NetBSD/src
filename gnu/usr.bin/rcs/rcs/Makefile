#	$NetBSD: Makefile,v 1.6 1997/03/24 22:06:42 christos Exp $

PROG=	rcs
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} ${LIBGNUMALLOC}

MAN=	rcs.1 rcsfile.5 rcsintro.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
