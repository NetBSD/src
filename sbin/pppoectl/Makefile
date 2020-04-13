#	$NetBSD: Makefile,v 1.3.78.2 2020/04/13 08:03:22 martin Exp $

PROG=	pppoectl
MAN=	pppoectl.8

USE_FORT?= yes	# network client

COPTS.pppoectl.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
