#	$NetBSD: Makefile,v 1.11 2023/06/03 09:09:11 lukem Exp $

.include <bsd.own.mk>
PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8
MLINKS = rpc.rquotad.8 rquotad.8

DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

COPTS.rquotad.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
