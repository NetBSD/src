#	$NetBSD: Makefile,v 1.13 2023/06/03 09:09:25 lukem Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?= yes	# network client

PROG=	ypset
MAN=	ypset.8

COPTS.ypset.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
