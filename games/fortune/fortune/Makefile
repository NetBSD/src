#	$Id: Makefile,v 1.6 1993/11/11 01:01:21 jtc Exp $

PROG=		fortune
MAN6=		fortune.0
CFLAGS+=	-I${.CURDIR}/../strfile 
LDADD+=		-lcompat
DPADD+=		${LIBCOMPAT}

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.prog.mk>
