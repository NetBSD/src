#	$NetBSD: Makefile,v 1.2.4.2 2019/06/10 22:09:15 christos Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	compat_crypto_50
SRCS=	ocryptodev.c

.include <bsd.kmodule.mk>
