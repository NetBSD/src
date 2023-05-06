#	$NetBSD: sm4.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/sm4

SM4_SRCS += \
sm4.c \

SRCS += ${SM4_SRCS}

.for cryptosrc in ${SM4_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sm4 ${SM4CPPFLAGS}
.endfor
