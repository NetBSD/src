#	$NetBSD: Makefile,v 1.4.18.1 2019/06/10 22:09:20 christos Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
IOCONF=	crypto.ioconf
SRCS=	cryptodev.c

WARNS=	3

.include <bsd.kmodule.mk>
