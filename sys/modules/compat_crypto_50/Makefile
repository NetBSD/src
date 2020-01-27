#	$NetBSD: Makefile,v 1.3 2020/01/27 17:09:17 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	compat_crypto_50
SRCS=	ocryptodev.c	compat_crypto_50.c

.include <bsd.kmodule.mk>
