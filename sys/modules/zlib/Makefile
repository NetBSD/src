#	$NetBSD: Makefile,v 1.5 2023/06/03 21:30:24 lukem Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	zlib
SRCS=	zlib.c

WARNS=	3

COPTS.zlib.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

.include <bsd.kmodule.mk>
