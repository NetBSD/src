#	$NetBSD: Makefile,v 1.5 2019/01/27 02:08:47 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
IOCONF=	crypto.ioconf
SRCS=	cryptodev.c

.include <bsd.kmodule.mk>
