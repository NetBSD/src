#	$NetBSD: sm3.inc,v 1.1.14.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/sm3

SM3_SRCS += \
legacy_sm3.c \
sm3.c \

SRCS += ${SM3_SRCS}

.for cryptosrc in ${SM3_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sm3 ${SM3CPPFLAGS}
.endfor
