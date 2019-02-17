#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:58 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	swcrypto
IOCONF=	swcrypto.ioconf
SRCS=	aesxcbcmac.c cryptosoft.c deflate.c gmac.c

WARNS=	3

.include <bsd.kmodule.mk>
