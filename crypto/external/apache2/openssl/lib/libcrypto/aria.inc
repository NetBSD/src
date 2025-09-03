#	$NetBSD: aria.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/aria

ARIA_SRCS += \
aria.c \

SRCS += ${ARIA_SRCS}

.for cryptosrc in ${ARIA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/aria ${ARIACPPFLAGS}
.endfor
