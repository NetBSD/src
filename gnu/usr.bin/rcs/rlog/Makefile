#	$Id: Makefile,v 1.3 1993/11/03 16:10:01 cgd Exp $

PROG=	rlog
CFLAGS+= -I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} /usr/lib/libgnumalloc.a

.include "../../Makefile.inc"
.include <bsd.prog.mk>
