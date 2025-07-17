#	$NetBSD: bf.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/bf

BF_SRCS ?= bf_enc.c

BF_SRCS += \
bf_cfb64.c \
bf_ecb.c \
bf_ofb64.c \
bf_skey.c \

SRCS += ${BF_SRCS}

.for cryptosrc in ${BF_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/bf ${BFCPPFLAGS}
.endfor
