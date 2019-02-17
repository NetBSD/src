#	$NetBSD: Makefile,v 1.6 2019/02/17 04:05:46 rin Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
IOCONF=	crypto.ioconf
SRCS=	cryptodev.c

WARNS=	3

.include <bsd.kmodule.mk>
