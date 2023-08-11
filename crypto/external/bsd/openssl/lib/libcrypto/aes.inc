#	$NetBSD: aes.inc,v 1.4.14.1 2023/08/11 13:41:54 martin Exp $

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

.for cryptosrc in ${AES_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/aes ${AESCPPFLAGS}
.endfor
