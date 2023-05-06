#	$NetBSD: camellia.inc,v 1.5 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/camellia

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
