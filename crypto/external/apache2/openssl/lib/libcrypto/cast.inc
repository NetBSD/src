#	$NetBSD: cast.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/cast

CAST_SRCS ?= c_enc.c

CAST_SRCS += \
c_cfb64.c \
c_ecb.c \
c_ofb64.c \
c_skey.c \

SRCS += ${CAST_SRCS}

.for cryptosrc in ${CAST_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/cast ${CASTCPPFLAGS}
.endfor
