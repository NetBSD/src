#	$NetBSD: Makefile,v 1.2 2014/01/31 18:12:15 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

IOCONF=	crypto.ioconf

KMOD=	crypto
SRCS=	cryptodev.c ocryptodev.c

.include <bsd.kmodule.mk>
