#	$NetBSD: Makefile,v 1.12 2023/06/03 09:09:12 lukem Exp $

PROG =	rpc.rwalld
SRCS =	rwalld.c
MAN =	rpc.rwalld.8
MLINKS=	rpc.rwalld.8 rwalld.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rwalld.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
