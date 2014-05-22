#	$NetBSD: Makefile,v 1.1.6.2 2014/05/22 11:41:06 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/crypto/des

KMOD=	des
SRCS=	des_module.c
SRCS+=	des_ecb.c
SRCS+=	des_setkey.c
SRCS+=	des_enc.c
SRCS+=	des_cbc.c


.include <bsd.kmodule.mk>
