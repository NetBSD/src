#       $NetBSD: camellia.inc,v 1.1.6.1 2012/10/30 18:47:27 yamt Exp $

.PATH:  ${OPENSSLSRC}/crypto/camellia

CAMELLIA_SRCS = camellia.c cmll_misc.c \
    cmll_ecb.c cmll_cbc.c cmll_cfb.c cmll_ofb.c cmll_ctr.c cmll_utl.c
SRCS += ${CAMELLIA_SRCS}

.for cryptosrc in ${CAMELLIA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/camellia
.endfor
