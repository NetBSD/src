#	$NetBSD: mdc2.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/mdc2

MDC2_SRCS += \
mdc2_one.c \
mdc2dgst.c \

SRCS += ${MDC2_SRCS}

.for cryptosrc in ${MDC2_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/mdc2 ${MDC2CPPFLAGS}
.endfor
