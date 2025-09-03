#	$NetBSD: poly1305.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/poly1305

POLY1305_SRCS += \
poly1305.c \

SRCS += ${POLY1305_SRCS}

.for cryptosrc in ${POLY1305_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/poly1305 ${POLY1305CPPFLAGS}
.endfor
