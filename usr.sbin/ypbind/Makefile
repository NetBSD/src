#	$NetBSD: Makefile,v 1.18 2009/04/22 15:23:10 lukem Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?= yes	# network client

PROG=	ypbind
MAN=	ypbind.8
CPPFLAGS+=-DHEURISTIC -DDEBUG

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
