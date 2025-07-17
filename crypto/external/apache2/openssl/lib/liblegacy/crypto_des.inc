#	$NetBSD: crypto_des.inc,v 1.1 2025/07/17 14:26:12 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/des

CRYPTO_DES_SRCS += \
des_enc.c \
fcrypt_b.c \

SRCS += ${CRYPTO_DES_SRCS}

.for legacysrc in ${CRYPTO_DES_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/crypto/des ${CRYPTO_DESCPPFLAGS}
.endfor
