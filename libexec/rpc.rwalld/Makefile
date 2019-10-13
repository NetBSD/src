#	$NetBSD: Makefile,v 1.11 2019/10/13 07:28:11 mrg Exp $

PROG =	rpc.rwalld
SRCS =	rwalld.c
MAN =	rpc.rwalld.8
MLINKS=	rpc.rwalld.8 rwalld.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rwalld.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
