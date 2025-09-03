#	$NetBSD: dsa.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/dsa

DSA_SRCS += \
dsa_ameth.c \
dsa_asn1.c \
dsa_backend.c \
dsa_check.c \
dsa_depr.c \
dsa_err.c \
dsa_gen.c \
dsa_key.c \
dsa_lib.c \
dsa_meth.c \
dsa_ossl.c \
dsa_pmeth.c \
dsa_prn.c \
dsa_sign.c \
dsa_vrf.c \

SRCS += ${DSA_SRCS}

.for cryptosrc in ${DSA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/dsa ${DSACPPFLAGS}
.endfor
