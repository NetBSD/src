#	$NetBSD: mdc2.inc,v 1.5 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/mdc2

MDC2_SRCS += \
mdc2_one.c \
mdc2dgst.c \

SRCS += ${MDC2_SRCS}

.for cryptosrc in ${MDC2_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/mdc2 ${MDC2CPPFLAGS}
.endfor
