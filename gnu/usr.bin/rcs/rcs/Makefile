#	$Id: Makefile,v 1.2 1993/08/02 17:47:41 mycroft Exp $

PROG=	rcs
CFLAGS+= -I${.CURDIR}/../lib
LDADD=  ${LIBRCS}
DPADD=  ${LIBRCS}

MAN1=	rcs.0 rcsintro.0
MAN5=   rcsfile.0

.include "../../Makefile.inc"
.include <bsd.prog.mk>
