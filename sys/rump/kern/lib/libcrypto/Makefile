#	$NetBSD: Makefile,v 1.17 2020/08/21 06:27:41 riastradh Exp $
#

SODIUM_IMPORTDIR=${.CURDIR}/../../../../external/isc/libsodium
SODIUM_DIR=${.CURDIR}/../../../../external/isc/libsodium/dist/src/libsodium

.PATH:	${.CURDIR}/../../../../crypto/adiantum				\
	${.CURDIR}/../../../../crypto/aes				\
	${.CURDIR}/../../../../crypto/blake2				\
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

# BLAKE2
SRCS+=	blake2s.c

# Various cryptography functions
SODIUM_CPPFLAGS=
SODIUM_CPPFLAGS+=	-I${SODIUM_IMPORTDIR}/include
SODIUM_CPPFLAGS+=	-I${SODIUM_IMPORTDIR}/dist/src/libsodium/include/sodium
SODIUM_CPPFLAGS+=	-Wno-shadow -Wno-unused-function -Wno-unused-variable -DHAVE_TI_MODE

CPPFLAGS.x25519_ref10.c+=		${SODIUM_CPPFLAGS}
CPPFLAGS.scalarmult_curve25519.c+=	${SODIUM_CPPFLAGS}
CPPFLAGS.crypto_scalarmult.c+=		${SODIUM_CPPFLAGS}
CPPFLAGS.poly1305_donna.c+=		${SODIUM_CPPFLAGS}
CPPFLAGS.onetimeauth_poly1305.c+=	${SODIUM_CPPFLAGS}
CPPFLAGS.crypto_onetimeauth.c+=		${SODIUM_CPPFLAGS}
CPPFLAGS.chacha20_ref.c+=		${SODIUM_CPPFLAGS}
CPPFLAGS.stream_chacha20.c+=		${SODIUM_CPPFLAGS}
CPPFLAGS.aead_xchacha20poly1305.c+=	${SODIUM_CPPFLAGS}
CPPFLAGS.aead_chacha20poly1305.c+=	${SODIUM_CPPFLAGS}
CPPFLAGS.core_hchacha20.c+=		${SODIUM_CPPFLAGS}
CPPFLAGS.ed25519_ref10.c+=		${SODIUM_CPPFLAGS}

SRCS+=	x25519_ref10.c scalarmult_curve25519.c crypto_scalarmult.c
SRCS+=	poly1305_donna.c onetimeauth_poly1305.c
SRCS+=	crypto_onetimeauth.c chacha20_ref.c stream_chacha20.c
SRCS+=	aead_xchacha20poly1305.c aead_chacha20poly1305.c
SRCS+=	core_hchacha20.c ed25519_ref10.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
