#	$NetBSD: async.inc,v 1.1.14.1 2023/08/11 13:41:54 martin Exp $

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
