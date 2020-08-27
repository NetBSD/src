#	$NetBSD: Makefile,v 1.21 2020/08/27 14:01:36 riastradh Exp $
#

SODIUM_IMPORTDIR=${.CURDIR}/../../../../external/isc/libsodium
SODIUM_DIR=${.CURDIR}/../../../../external/isc/libsodium/dist/src/libsodium

.PATH:	${.CURDIR}/../../../../crypto/adiantum				\
	${.CURDIR}/../../../../crypto/aes				\
	${.CURDIR}/../../../../crypto/blowfish				\
	${.CURDIR}/../../../../crypto/camellia				\
	${.CURDIR}/../../../../crypto/cast128				\
	${.CURDIR}/../../../../crypto/des				\
	${.CURDIR}/../../../../crypto/skipjack				\
	${SODIUM_DIR}/crypto_scalarmult/curve25519/ref10		\
	${SODIUM_DIR}/crypto_scalarmult/curve25519			\
	${SODIUM_DIR}/crypto_scalarmult					\
	${SODIUM_DIR}/crypto_onetimeauth/poly1305/donna			\
	${SODIUM_DIR}/crypto_onetimeauth/poly1305			\
	${SODIUM_DIR}/crypto_onetimeauth				\
	${SODIUM_DIR}/crypto_stream/chacha20/ref			\
	${SODIUM_DIR}/crypto_stream/chacha20				\
	${SODIUM_DIR}/crypto_aead/xchacha20poly1305/sodium		\
	${SODIUM_DIR}/crypto_aead/chacha20poly1305/sodium		\
	${SODIUM_DIR}/crypto_core/hchacha20				\
	${SODIUM_DIR}/crypto_core/ed25519/ref10

LIB=	rumpkern_crypto
COMMENT=Cryptographic routines

# Adiantum
SRCS+=	adiantum.c
SRCS+=	adiantum_selftest.c

# AES
SRCS+=	aes_bear.c
SRCS+=	aes_ccm.c
SRCS+=	aes_ccm_mbuf.c
SRCS+=	aes_ct.c
SRCS+=	aes_ct_dec.c
SRCS+=	aes_ct_enc.c
SRCS+=	aes_impl.c
SRCS+=	aes_selftest.c

# blowfish
SRCS+=	bf_ecb.c bf_enc.c bf_cbc.c bf_skey.c bf_module.c

# camellia
SRCS+=  camellia.c camellia-api.c

# cast128
SRCS+=	cast128.c

# DES
SRCS+=	des_ecb.c des_setkey.c des_enc.c des_cbc.c des_module.c

# skipjack
SRCS+=	skipjack.c

# libsodium
SODIUM_CPPFLAGS+=	-I${SODIUM_IMPORTDIR}/include
SODIUM_CPPFLAGS+=	-I${SODIUM_IMPORTDIR}/dist/src/libsodium/include/sodium

#SODIUM_CPPFLAGS+=	-DHAVE_TI_MODE

SODIUM_CWARNFLAGS+=	-Wno-shadow
SODIUM_CWARNFLAGS+=	-Wno-unused-function
SODIUM_CWARNFLAGS+=	-Wno-unused-variable

SODIUM_SRCS+=	x25519_ref10.c
SODIUM_SRCS+=	scalarmult_curve25519.c
SODIUM_SRCS+=	crypto_scalarmult.c
SODIUM_SRCS+=	poly1305_donna.c
SODIUM_SRCS+=	onetimeauth_poly1305.c
SODIUM_SRCS+=	crypto_onetimeauth.c
SODIUM_SRCS+=	chacha20_ref.c
SODIUM_SRCS+=	stream_chacha20.c
SODIUM_SRCS+=	aead_xchacha20poly1305.c
SODIUM_SRCS+=	aead_chacha20poly1305.c
SODIUM_SRCS+=	core_hchacha20.c
SODIUM_SRCS+=	ed25519_ref10.c

SRCS+=	${SODIUM_SRCS}

.for _s_ in ${SODIUM_SRCS}
CPPFLAGS.${_s_}+=	${SODIUM_CPPFLAGS}
COPTS.${_s_}+=		${SODIUM_CWARNFLAGS}
.endfor

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
