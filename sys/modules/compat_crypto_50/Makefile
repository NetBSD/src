#	$NetBSD: Makefile,v 1.2.4.3 2020/04/08 14:08:54 martin Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	compat_crypto_50
SRCS=	ocryptodev.c	compat_crypto_50.c

.include <bsd.kmodule.mk>
