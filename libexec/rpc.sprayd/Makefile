#	$NetBSD: Makefile,v 1.8 2023/06/03 09:09:12 lukem Exp $

PROG =	rpc.sprayd
SRCS =	sprayd.c
MAN =	rpc.sprayd.8
MLINKS=	rpc.sprayd.8 sprayd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.sprayd.c+=	${CC_WNO_STRINGOP_TRUNCATION} ${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
