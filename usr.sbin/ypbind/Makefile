#	$NetBSD: Makefile,v 1.17 2007/07/07 22:33:57 christos Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?= yes	# network client
WARNS?=	4

PROG=	ypbind
MAN=	ypbind.8
CPPFLAGS+=-DHEURISTIC -DDEBUG

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
