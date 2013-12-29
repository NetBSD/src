#	$NetBSD: Makefile,v 1.1 2013/12/29 18:20:42 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	swcrypto
SRCS=	aesxcbcmac.c cryptosoft.c deflate.c gmac.c

.include <bsd.kmodule.mk>
