#	$NetBSD: thread.inc,v 1.1 2025/07/21 22:21:21 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/thread ${OPENSSLSRC}/crypto/thread/arch

THREAD_SRCS += \
thread_posix.c

SRCS += ${THREAD_SRCS}

.for cryptosrc in ${THREAD_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/thread
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/thread/arch ${THREADCPPFLAGS}
.endfor
