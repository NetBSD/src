#	$NetBSD: Makefile,v 1.1.10.1 2002/01/10 19:50:39 thorpej Exp $

PROG=		newdisk
CPPFLAGS+=	-DMBOOT=\"/usr/mdec/mboot\"
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
LDSTATIC=	-static
NOMAN=		# defined

BINDIR=		/usr/mdec

.include <bsd.prog.mk>
