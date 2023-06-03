#	$NetBSD: Makefile,v 1.12 2023/06/03 09:09:18 lukem Exp $

USE_FORT?= yes	# network client

PROG = rwall

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rwall.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
