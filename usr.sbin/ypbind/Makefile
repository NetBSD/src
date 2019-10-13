#	$NetBSD: Makefile,v 1.19 2019/10/13 07:28:22 mrg Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?= yes	# network client

PROG=	ypbind
MAN=	ypbind.8
CPPFLAGS+=-DHEURISTIC -DDEBUG

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.ypbind.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
