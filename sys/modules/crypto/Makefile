#	$NetBSD: Makefile,v 1.4 2015/08/20 11:05:01 christos Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
IOCONF=	crypto.ioconf
SRCS=	cryptodev.c ocryptodev.c

.include <bsd.kmodule.mk>
