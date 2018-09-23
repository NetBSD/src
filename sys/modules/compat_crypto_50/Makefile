#	$NetBSD: Makefile,v 1.1.2.1 2018/09/23 01:33:26 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
SRCS=	ocryptodev.c ocryptodev.c

.include <bsd.kmodule.mk>
