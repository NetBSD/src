#	$NetBSD: Makefile,v 1.2.8.1 2020/02/29 20:21:04 ad Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	compat_crypto_50
SRCS=	ocryptodev.c	compat_crypto_50.c

.include <bsd.kmodule.mk>
