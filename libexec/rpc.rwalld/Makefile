#	$NetBSD: Makefile,v 1.10.110.1 2020/04/13 08:03:18 martin Exp $

PROG =	rpc.rwalld
SRCS =	rwalld.c
MAN =	rpc.rwalld.8
MLINKS=	rpc.rwalld.8 rwalld.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rwalld.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
