#	$NetBSD: sm2.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/sm2

SM2_SRCS += \
sm2_crypt.c \
sm2_err.c \
sm2_key.c \
sm2_sign.c \

SRCS += ${SM2_SRCS}

.for cryptosrc in ${SM2_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sm2 ${SM2CPPFLAGS}
.endfor
