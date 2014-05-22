#	$NetBSD: Makefile,v 1.3.6.2 2014/05/22 11:41:06 yamt Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
SRCS=	cryptodev.c ocryptodev.c

.include <bsd.kmodule.mk>
