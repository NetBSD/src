#	$NetBSD: thread.inc,v 1.1.2.1 2025/11/20 18:46:44 martin Exp $

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

CPPFLAGS.thread_posix.c+=	-D_NETBSD_PTHREAD_CREATE_WEAK
