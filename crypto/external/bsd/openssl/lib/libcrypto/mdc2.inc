#	$NetBSD: mdc2.inc,v 1.4.20.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/mdc2

MDC2_SRCS += \
mdc2_one.c \
mdc2dgst.c \

SRCS += ${MDC2_SRCS}

.for cryptosrc in ${MDC2_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/mdc2 ${MDC2CPPFLAGS}
.endfor
