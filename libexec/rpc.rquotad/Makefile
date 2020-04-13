#	$NetBSD: Makefile,v 1.9.40.1 2020/04/13 08:03:18 martin Exp $

.include <bsd.own.mk>
PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8
MLINKS = rpc.rquotad.8 rquotad.8

DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

COPTS.rquotad.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
