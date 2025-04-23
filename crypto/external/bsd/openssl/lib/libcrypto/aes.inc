#	$NetBSD: aes.inc,v 1.6 2025/04/23 20:14:59 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/aes

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
