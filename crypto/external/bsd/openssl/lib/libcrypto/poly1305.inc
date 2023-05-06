#	$NetBSD: poly1305.inc,v 1.3 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/poly1305

POLY1305_SRCS += \
poly1305.c \

SRCS += ${POLY1305_SRCS}

.for cryptosrc in ${POLY1305_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/poly1305 ${POLY1305CPPFLAGS}
.endfor
