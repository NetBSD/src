#	$NetBSD: aria.inc,v 1.1.14.1 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/aria

ARIA_SRCS += \
aria.c \

SRCS += ${ARIA_SRCS}

.for cryptosrc in ${ARIA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/aria ${ARIACPPFLAGS}
.endfor
