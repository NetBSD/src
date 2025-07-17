#	$NetBSD: camellia.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/camellia

CAMELLIA_SRCS?= camellia.c cmll_cbc.c

CAMELLIA_SRCS += \
cmll_cfb.c \
cmll_ctr.c \
cmll_ecb.c \
cmll_misc.c \
cmll_ofb.c \

SRCS += ${CAMELLIA_SRCS}

.for cryptosrc in ${CAMELLIA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/camellia ${CAMELLIACPPFLAGS}
.endfor
