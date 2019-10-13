#	$NetBSD: Makefile,v 1.9 2019/10/13 07:28:16 mrg Exp $

USE_FORT?= yes	# network client

PROG=	rup

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rup.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
