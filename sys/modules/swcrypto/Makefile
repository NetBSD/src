#	$NetBSD: Makefile,v 1.2 2015/08/20 11:05:01 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	swcrypto
IOCONF=	swcrypto.ioconf
SRCS=	aesxcbcmac.c cryptosoft.c deflate.c gmac.c

.include <bsd.kmodule.mk>
