#	$NetBSD: Makefile,v 1.9.2.1 1997/11/08 06:00:43 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=		fortune
MAN=		fortune.6
CPPFLAGS+=	-I${.CURDIR}/../strfile 
LDADD+=		-lcompat
DPADD+=		${LIBCOMPAT}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
