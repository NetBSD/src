#	$NetBSD: ml_dsa.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/ml_dsa

ML_DSA_SRCS += \
ml_dsa_encoders.c \
ml_dsa_key.c \
ml_dsa_key_compress.c \
ml_dsa_matrix.c \
ml_dsa_ntt.c \
ml_dsa_params.c \
ml_dsa_sample.c \
ml_dsa_sign.c \

SRCS += ${ML_DSA_SRCS}

.for cryptosrc in ${ML_DSA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ml_dsa ${ML_DSACPPFLAGS}
.endfor
