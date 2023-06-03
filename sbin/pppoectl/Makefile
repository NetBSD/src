#	$NetBSD: Makefile,v 1.6 2023/06/03 09:09:13 lukem Exp $

PROG=	pppoectl
MAN=	pppoectl.8

USE_FORT?= yes	# network client

COPTS.pppoectl.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
