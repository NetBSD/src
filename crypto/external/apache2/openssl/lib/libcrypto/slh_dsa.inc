#	$NetBSD: slh_dsa.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/slh_dsa

SLH_DSA_SRCS += \
slh_adrs.c \
slh_dsa.c \
slh_dsa_hash_ctx.c \
slh_dsa_key.c \
slh_fors.c \
slh_hash.c \
slh_hypertree.c \
slh_params.c \
slh_wots.c \
slh_xmss.c \

SRCS += ${SLH_DSA_SRCS}

.for cryptosrc in ${SLH_DSA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/slh_dsa ${SLH_DSACPPFLAGS}
.endfor
