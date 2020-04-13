#	$NetBSD: Makefile,v 1.10.46.1 2020/04/13 08:05:47 martin Exp $

USE_FORT?= yes	# network client

PROG = rwall

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rwall.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
