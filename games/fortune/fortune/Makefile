#	$NetBSD: Makefile,v 1.9 1997/10/10 13:04:36 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=		fortune
MAN=		fortune.6
CFLAGS+=	-I${.CURDIR}/../strfile 
LDADD+=		-lcompat
DPADD+=		${LIBCOMPAT}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
