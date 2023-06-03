#	$NetBSD: Makefile,v 1.3 2023/06/03 09:09:08 lukem Exp $

BASE=	${.CURDIR:T}

.include "${.CURDIR}/../Makefile.inc"

COPTS.discover.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
