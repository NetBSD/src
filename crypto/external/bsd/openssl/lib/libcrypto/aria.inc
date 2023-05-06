#	$NetBSD: aria.inc,v 1.2 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/aria

ARIA_SRCS += \
aria.c \

SRCS += ${ARIA_SRCS}

.for cryptosrc in ${ARIA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/aria ${ARIACPPFLAGS}
.endfor
