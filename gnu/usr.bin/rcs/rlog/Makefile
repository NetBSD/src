#	$NetBSD: Makefile,v 1.5 1997/03/24 22:06:47 christos Exp $

PROG=	rlog
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} ${LIBGNUMALLOC}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
