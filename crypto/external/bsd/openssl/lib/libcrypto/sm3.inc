#	$NetBSD: sm3.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/sm3

SM3_SRCS += \
legacy_sm3.c \
sm3.c \

SRCS += ${SM3_SRCS}

.for cryptosrc in ${SM3_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sm3 ${SM3CPPFLAGS}
.endfor
