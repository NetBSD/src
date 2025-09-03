#	$NetBSD: thread.inc,v 1.1 2025/07/17 14:25:27 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/thread ${OPENSSLSRC}/crypto/thread/arch

THREAD_SRCS += \
api.c \
arch.c \
internal.c \
thread_none.c \
thread_posix.c \
thread_win.c \

SRCS += ${THREAD_SRCS}

.for cryptosrc in ${THREAD_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/thread
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/thread/arch ${THREADCPPFLAGS}
.endfor
