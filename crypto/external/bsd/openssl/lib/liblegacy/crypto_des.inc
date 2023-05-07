#	$NetBSD: crypto_des.inc,v 1.1 2023/05/07 16:22:10 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/des

CRYPTO_DES_SRCS += \
des_enc.c \
fcrypt_b.c \

SRCS += ${CRYPTO_DES_SRCS}

.for legacysrc in ${CRYPTO_DES_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/crypto/des ${CRYPTO_DESCPPFLAGS}
.endfor
