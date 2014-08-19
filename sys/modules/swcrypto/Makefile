#	$NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:04:34 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	swcrypto
SRCS=	aesxcbcmac.c cryptosoft.c deflate.c gmac.c

.include <bsd.kmodule.mk>
