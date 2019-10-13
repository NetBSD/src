#	$NetBSD: Makefile,v 1.5 2019/10/13 07:28:12 mrg Exp $

PROG=	pppoectl
MAN=	pppoectl.8

USE_FORT?= yes	# network client

COPTS.pppoectl.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
