#	$NetBSD: Makefile,v 1.1 2014/01/01 15:18:57 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/crypto/des

KMOD=	des
SRCS=	des_module.c
SRCS+=	des_ecb.c
SRCS+=	des_setkey.c
SRCS+=	des_enc.c
SRCS+=	des_cbc.c


.include <bsd.kmodule.mk>
