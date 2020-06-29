#	$NetBSD: Makefile,v 1.8 2020/06/29 23:44:01 riastradh Exp $
#

.PATH:	${.CURDIR}/../../../../crypto/adiantum				\
	${.CURDIR}/../../../../crypto/aes				\
	${.CURDIR}/../../../../crypto/blowfish				\
	${.CURDIR}/../../../../crypto/camellia				\
	${.CURDIR}/../../../../crypto/cast128				\
	${.CURDIR}/../../../../crypto/des				\
	${.CURDIR}/../../../../crypto/skipjack

LIB=	rumpkern_crypto
COMMENT=Cryptographic routines

# Adiantum
SRCS+=	adiantum.c
SRCS+=	adiantum_selftest.c

# blowfish
SRCS+=	bf_ecb.c bf_enc.c bf_cbc.c bf_skey.c bf_module.c

# camellia
SRCS+=  camellia.c camellia-api.c

# cast128
SRCS+=	cast128.c

# DES
SRCS+=	des_ecb.c des_setkey.c des_enc.c des_cbc.c des_module.c

# AES
SRCS+=	aes_bear.c
SRCS+=	aes_ct.c
SRCS+=	aes_ct_dec.c
SRCS+=	aes_ct_enc.c
SRCS+=	aes_impl.c
SRCS+=	aes_rijndael.c
SRCS+=	aes_selftest.c

# skipjack
SRCS+=	skipjack.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
