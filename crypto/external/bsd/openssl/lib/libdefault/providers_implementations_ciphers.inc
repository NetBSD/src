#	$NetBSD: providers_implementations_ciphers.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/ciphers

PROVIDERS_IMPLEMENTATIONS_CIPHERS_SRCS += \
cipher_aes.c \
cipher_aes_cbc_hmac_sha.c \
cipher_aes_cbc_hmac_sha1_hw.c \
cipher_aes_cbc_hmac_sha256_hw.c \
cipher_aes_ccm.c \
cipher_aes_ccm_hw.c \
cipher_aes_gcm.c \
cipher_aes_gcm_hw.c \
cipher_aes_hw.c \
cipher_aes_ocb.c \
cipher_aes_ocb_hw.c \
cipher_aes_siv.c \
cipher_aes_siv_hw.c \
cipher_aes_wrp.c \
cipher_aes_xts.c \
cipher_aes_xts_fips.c \
cipher_aes_xts_hw.c \
cipher_aria.c \
cipher_aria_ccm.c \
cipher_aria_ccm_hw.c \
cipher_aria_gcm.c \
cipher_aria_gcm_hw.c \
cipher_aria_hw.c \
cipher_camellia.c \
cipher_camellia_hw.c \
cipher_chacha20.c \
cipher_chacha20_hw.c \
cipher_chacha20_poly1305.c \
cipher_chacha20_poly1305_hw.c \
cipher_cts.c \
cipher_null.c \
cipher_sm4.c \
cipher_sm4_hw.c \
cipher_tdes.c \
cipher_tdes_common.c \
cipher_tdes_default.c \
cipher_tdes_default_hw.c \
cipher_tdes_hw.c \
cipher_tdes_wrap.c \
cipher_tdes_wrap_hw.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_CIPHERS_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_CIPHERS_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/ciphers ${PROVIDERS_IMPLEMENTATIONS_CIPHERSCPPFLAGS}
.endfor
