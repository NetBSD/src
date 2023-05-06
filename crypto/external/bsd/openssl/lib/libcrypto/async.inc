#	$NetBSD: async.inc,v 1.2 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/async ${OPENSSLSRC}/crypto/async/arch

ASYNC_SRCS += \
async.c \
async_err.c \
async_posix.c \
async_wait.c \

SRCS += ${ASYNC_SRCS}

.for cryptosrc in ${ASYNC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/async ${ASYNCCPPFLAGS}
.endfor
