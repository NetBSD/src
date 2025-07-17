#	$NetBSD: ml_kem.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/ml_kem

ML_KEM_SRCS += \
ml_kem.c \

SRCS += ${ML_KEM_SRCS}

.for cryptosrc in ${ML_KEM_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ml_kem ${ML_KEMCPPFLAGS}
.endfor
