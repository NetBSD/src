#	$NetBSD: aes.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/aes

AES_SRCS ?= aes_core.c aes_cbc.c

AES_SRCS += \
aes_cfb.c \
aes_ecb.c \
aes_ige.c \
aes_misc.c \
aes_ofb.c \
aes_wrap.c \

SRCS += ${AES_SRCS}

# need for all files
CPPFLAGS+=${AESCPPFLAGS}
