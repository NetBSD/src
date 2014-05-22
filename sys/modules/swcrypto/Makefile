#	$NetBSD: Makefile,v 1.1.6.2 2014/05/22 11:41:08 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	swcrypto
SRCS=	aesxcbcmac.c cryptosoft.c deflate.c gmac.c

.include <bsd.kmodule.mk>
