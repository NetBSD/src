#	$NetBSD: Makefile,v 1.1 2013/12/29 18:20:42 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
SRCS=	cryptodev.c ocryptodev.c

.include <bsd.kmodule.mk>
