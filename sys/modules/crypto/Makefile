#	$NetBSD: Makefile,v 1.3.12.1 2015/09/22 12:06:09 skrll Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
IOCONF=	crypto.ioconf
SRCS=	cryptodev.c ocryptodev.c

.include <bsd.kmodule.mk>
