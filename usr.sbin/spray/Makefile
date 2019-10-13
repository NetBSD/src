#	$NetBSD: Makefile,v 1.5 2019/10/13 07:28:21 mrg Exp $

PROG=	spray
MAN=	spray.8
LDADD+=	-lrpcsvc
DPADD+=	${LIBRPCSVC}

COPTS.spray.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
