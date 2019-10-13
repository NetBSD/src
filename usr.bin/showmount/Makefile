#	$NetBSD: Makefile,v 1.8 2019/10/13 07:28:17 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

USE_FORT?= yes	# network client

PROG=	showmount
MAN=	showmount.8

COPTS.showmount.c+=	${GCC_NO_STRINGOP_OVERFLOW} ${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
