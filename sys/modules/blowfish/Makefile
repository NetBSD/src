#	$NetBSD: Makefile,v 1.1 2014/01/01 15:18:57 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/crypto/blowfish

KMOD=	blowfish
SRCS=	bf_module.c
SRCS+=	bf_ecb.c
SRCS+=	bf_skey.c  

.if ${MACHINE} == "i386"

.PATH:	${S}/crypto/blowfish/arch/i386

SRCS+=	bf_enc.S
SRCS+=	bf_cbc.S

.else

SRCS+=	bf_enc.c
SRCS+=	bf_cbc.c

.endif

.include <bsd.kmodule.mk>
