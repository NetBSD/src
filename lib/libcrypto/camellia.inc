#       $NetBSD: camellia.inc,v 1.1 2008/05/26 16:51:07 christos Exp $

.PATH:  ${OPENSSLSRC}/crypto/camellia

CAMELLIA_SRCS = camellia.c cmll_misc.c \
    cmll_ecb.c cmll_cbc.c cmll_cfb.c cmll_ofb.c cmll_ctr.c
SRCS += ${CAMELLIA_SRCS}

.for cryptosrc in ${CAMELLIA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/camellia
.endfor
