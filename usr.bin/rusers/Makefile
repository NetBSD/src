#	$NetBSD: Makefile,v 1.13 2023/06/03 09:09:18 lukem Exp $

USE_FORT?= yes	# network client

PROG = rusers

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rusers.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
