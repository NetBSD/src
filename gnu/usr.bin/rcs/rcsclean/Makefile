#	$Id: Makefile,v 1.3 1993/11/03 16:09:38 cgd Exp $

PROG=	rcsclean
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} /usr/lib/libgnumalloc.a

.include "../../Makefile.inc"
.include <bsd.prog.mk>
