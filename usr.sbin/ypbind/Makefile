#	$NetBSD: Makefile,v 1.18.46.1 2020/04/13 08:06:07 martin Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?= yes	# network client

PROG=	ypbind
MAN=	ypbind.8
CPPFLAGS+=-DHEURISTIC -DDEBUG

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.ypbind.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
