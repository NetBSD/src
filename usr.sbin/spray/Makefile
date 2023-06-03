#	$NetBSD: Makefile,v 1.6 2023/06/03 09:09:24 lukem Exp $

PROG=	spray
MAN=	spray.8
LDADD+=	-lrpcsvc
DPADD+=	${LIBRPCSVC}

COPTS.spray.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
