#	$NetBSD: camellia.inc,v 1.4.14.1 2023/08/11 13:41:54 martin Exp $

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
