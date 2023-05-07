#	$NetBSD: providers_implementations_ciphers.inc,v 1.1 2023/05/07 16:22:10 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/ciphers

PROVIDERS_IMPLEMENTATIONS_CIPHERS_SRCS += \
cipher_blowfish.c \
cipher_blowfish_hw.c \
cipher_cast5.c \
cipher_cast5_hw.c \
cipher_des.c \
cipher_des_hw.c \
cipher_desx.c \
cipher_desx_hw.c \
cipher_idea.c \
cipher_idea_hw.c \
cipher_rc2.c \
cipher_rc2_hw.c \
cipher_rc4.c \
cipher_rc4_hmac_md5.c \
cipher_rc4_hmac_md5_hw.c \
cipher_rc4_hw.c \
cipher_seed.c \
cipher_seed_hw.c \
cipher_tdes_common.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_CIPHERS_SRCS}

.for legacysrc in ${PROVIDERS_IMPLEMENTATIONS_CIPHERS_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers/implementations/ciphers ${PROVIDERS_IMPLEMENTATIONS_CIPHERSCPPFLAGS}
.endfor
