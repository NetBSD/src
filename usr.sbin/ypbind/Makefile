#	$NetBSD: Makefile,v 1.20 2023/06/03 09:09:24 lukem Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?= yes	# network client

PROG=	ypbind
MAN=	ypbind.8
CPPFLAGS+=-DHEURISTIC -DDEBUG

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.ypbind.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
