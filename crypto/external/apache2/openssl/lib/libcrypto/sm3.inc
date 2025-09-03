#	$NetBSD: sm3.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/sm3

SM3_SRCS += \
legacy_sm3.c \
sm3.c \

SRCS += ${SM3_SRCS}

.for cryptosrc in ${SM3_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sm3 ${SM3CPPFLAGS}
.endfor
