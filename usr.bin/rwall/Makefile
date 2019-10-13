#	$NetBSD: Makefile,v 1.11 2019/10/13 07:28:16 mrg Exp $

USE_FORT?= yes	# network client

PROG = rwall

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rwall.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
