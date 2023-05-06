#	$NetBSD: buffer.inc,v 1.4 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/buffer

BUFFER_SRCS += \
buf_err.c \
buffer.c \

SRCS += ${BUFFER_SRCS}

.for cryptosrc in ${BUFFER_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/buffer ${BUFFERCPPFLAGS}
.endfor
