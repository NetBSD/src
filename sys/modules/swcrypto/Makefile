#	$NetBSD: Makefile,v 1.1.10.3 2017/12/03 11:38:59 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	swcrypto
IOCONF=	swcrypto.ioconf
SRCS=	aesxcbcmac.c cryptosoft.c deflate.c gmac.c

.include <bsd.kmodule.mk>
