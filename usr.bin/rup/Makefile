#	$NetBSD: Makefile,v 1.10 2023/06/03 09:09:18 lukem Exp $

USE_FORT?= yes	# network client

PROG=	rup

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rup.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
