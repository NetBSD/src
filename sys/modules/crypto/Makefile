#	$NetBSD: Makefile,v 1.3 2014/02/03 23:11:40 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
SRCS=	cryptodev.c ocryptodev.c

.include <bsd.kmodule.mk>
