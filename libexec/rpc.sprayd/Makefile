#	$NetBSD: Makefile,v 1.6.110.1 2020/04/13 08:03:18 martin Exp $

PROG =	rpc.sprayd
SRCS =	sprayd.c
MAN =	rpc.sprayd.8
MLINKS=	rpc.sprayd.8 sprayd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.sprayd.c+=	${GCC_NO_STRINGOP_TRUNCATION} ${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
