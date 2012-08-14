#       $NetBSD: camellia.inc,v 1.1.8.1 2012/08/14 07:57:02 riz Exp $

.PATH:  ${OPENSSLSRC}/crypto/camellia

CAMELLIA_SRCS = camellia.c cmll_misc.c \
    cmll_ecb.c cmll_cbc.c cmll_cfb.c cmll_ofb.c cmll_ctr.c cmll_utl.c
SRCS += ${CAMELLIA_SRCS}

.for cryptosrc in ${CAMELLIA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/camellia
.endfor
