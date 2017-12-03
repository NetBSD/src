#	$NetBSD: Makefile,v 1.3.10.3 2017/12/03 11:38:51 jdolecek Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
IOCONF=	crypto.ioconf
SRCS=	cryptodev.c ocryptodev.c

.include <bsd.kmodule.mk>
