#	$NetBSD: buffer.inc,v 1.3.14.1 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/buffer

BUFFER_SRCS += \
buf_err.c \
buffer.c \

SRCS += ${BUFFER_SRCS}

.for cryptosrc in ${BUFFER_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/buffer ${BUFFERCPPFLAGS}
.endfor
