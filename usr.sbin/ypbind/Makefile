#	$NetBSD: Makefile,v 1.15 1999/06/06 02:38:00 thorpej Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

PROG=	ypbind
MAN=	ypbind.8
CPPFLAGS+=-DHEURISTIC

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
