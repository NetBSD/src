#	$NetBSD: buffer.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/buffer

BUFFER_SRCS += \
buf_err.c \
buffer.c \

SRCS += ${BUFFER_SRCS}

.for cryptosrc in ${BUFFER_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/buffer ${BUFFERCPPFLAGS}
.endfor
