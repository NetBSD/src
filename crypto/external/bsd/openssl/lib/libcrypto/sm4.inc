#	$NetBSD: sm4.inc,v 1.1.14.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/sm4

SM4_SRCS += \
sm4.c \

SRCS += ${SM4_SRCS}

.for cryptosrc in ${SM4_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sm4 ${SM4CPPFLAGS}
.endfor
