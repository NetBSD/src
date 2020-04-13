#	$NetBSD: Makefile,v 1.11.76.1 2020/04/13 08:05:47 martin Exp $

USE_FORT?= yes	# network client

PROG = rusers

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rusers.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
