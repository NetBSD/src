#	$Id: Makefile,v 1.5 1993/08/05 02:16:36 jtc Exp $

PROG=		fortune
MAN6=		fortune.0
CFLAGS+=	-I${.CURDIR}/../strfile 
LDADD+=		-lgnuregex
DPADD+=		/usr/lib/libgnuregex.a

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.prog.mk>
