#	$NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:04:32 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/crypto/des

KMOD=	des
SRCS=	des_module.c
SRCS+=	des_ecb.c
SRCS+=	des_setkey.c
SRCS+=	des_enc.c
SRCS+=	des_cbc.c


.include <bsd.kmodule.mk>
