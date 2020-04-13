#	$NetBSD: Makefile,v 1.4.110.1 2020/04/13 08:05:58 martin Exp $

PROG=	spray
MAN=	spray.8
LDADD+=	-lrpcsvc
DPADD+=	${LIBRPCSVC}

COPTS.spray.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
