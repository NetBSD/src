#	$NetBSD: poly1305.inc,v 1.2.10.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/poly1305

POLY1305_SRCS += \
poly1305.c \

SRCS += ${POLY1305_SRCS}

.for cryptosrc in ${POLY1305_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/poly1305 ${POLY1305CPPFLAGS}
.endfor
