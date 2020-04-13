#	$NetBSD: Makefile,v 1.8.46.1 2020/04/13 08:05:46 martin Exp $

USE_FORT?= yes	# network client

PROG=	rup

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rup.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
