#	$NetBSD: Makefile,v 1.3.10.2 2014/08/20 00:04:32 tls Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
SRCS=	cryptodev.c ocryptodev.c

.include <bsd.kmodule.mk>
