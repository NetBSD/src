#	$NetBSD: async.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/async ${OPENSSLSRC}/crypto/async/arch

ASYNC_SRCS += \
async.c \
async_err.c \
async_null.c \
async_posix.c \
async_wait.c \
async_win.c \

SRCS += ${ASYNC_SRCS}

.for cryptosrc in ${ASYNC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/async ${ASYNCCPPFLAGS}
.endfor
