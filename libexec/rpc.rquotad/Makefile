#	$NetBSD: Makefile,v 1.10 2019/10/13 07:28:11 mrg Exp $

.include <bsd.own.mk>
PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8
MLINKS = rpc.rquotad.8 rquotad.8

DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

COPTS.rquotad.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
