#	$NetBSD: Makefile,v 1.5 1997/03/24 22:06:46 christos Exp $

PROG=	rcsmerge
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} ${LIBGNUMALLOC}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
