#	$NetBSD: Makefile,v 1.1.2.2 2018/09/23 03:44:04 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
SRCS=	ocryptodev.c

.include <bsd.kmodule.mk>
