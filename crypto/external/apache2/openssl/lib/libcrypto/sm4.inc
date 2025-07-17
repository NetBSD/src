#	$NetBSD: sm4.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/sm4

SM4_SRCS += \
sm4.c \

SRCS += ${SM4_SRCS}

.for cryptosrc in ${SM4_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sm4 ${SM4CPPFLAGS}
.endfor
