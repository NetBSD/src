#	$NetBSD: Makefile,v 1.9 2023/06/03 09:09:18 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

USE_FORT?= yes	# network client

PROG=	showmount
MAN=	showmount.8

COPTS.showmount.c+=	${CC_WNO_STRINGOP_OVERFLOW} ${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
