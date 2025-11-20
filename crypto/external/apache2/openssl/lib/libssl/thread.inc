#	$NetBSD: thread.inc,v 1.1.2.1 2025/11/20 18:46:43 martin Exp $

.PATH.c:	${OPENSSLSRC}/crypto/thread ${OPENSSLSRC}/crypto/thread/arch

THREAD_SRCS += \
thread_posix.c

SRCS += ${THREAD_SRCS}

.for cryptosrc in ${THREAD_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/thread
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/thread/arch ${THREADCPPFLAGS}
.endfor

CPPFLAGS.thread_posix.c+=	-D_NETBSD_PTHREAD_CREATE_WEAK
