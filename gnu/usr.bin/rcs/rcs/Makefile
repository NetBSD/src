#	$Id: Makefile,v 1.4 1994/12/22 09:45:30 cgd Exp $

PROG=	rcs
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} /usr/lib/libgnumalloc.a

MAN=	rcs.1 rcsfile.5 rcsintro.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
