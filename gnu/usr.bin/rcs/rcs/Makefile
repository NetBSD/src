#	$Id: Makefile,v 1.3 1993/11/03 16:09:31 cgd Exp $

PROG=	rcs
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} /usr/lib/libgnumalloc.a

MAN1=	rcs.0 rcsintro.0
MAN5=   rcsfile.0

.include "../../Makefile.inc"
.include <bsd.prog.mk>
