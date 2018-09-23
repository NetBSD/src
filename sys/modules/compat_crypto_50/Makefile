#	$NetBSD: Makefile,v 1.1.2.3 2018/09/23 04:31:17 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	compat_crypto_50
SRCS=	ocryptodev.c

.include <bsd.kmodule.mk>
