#	$NetBSD: Makefile,v 1.2 2019/01/27 02:08:45 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	compat_crypto_50
SRCS=	ocryptodev.c

.include <bsd.kmodule.mk>
