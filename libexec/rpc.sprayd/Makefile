#	$NetBSD: Makefile,v 1.7 2019/10/13 07:28:12 mrg Exp $

PROG =	rpc.sprayd
SRCS =	sprayd.c
MAN =	rpc.sprayd.8
MLINKS=	rpc.sprayd.8 sprayd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.sprayd.c+=	${GCC_NO_STRINGOP_TRUNCATION} ${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
