#	$NetBSD: Makefile,v 1.2.18.1 2019/06/10 22:09:39 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	swcrypto
IOCONF=	swcrypto.ioconf
SRCS=	aesxcbcmac.c cryptosoft.c deflate.c gmac.c

WARNS=	3

.include <bsd.kmodule.mk>
