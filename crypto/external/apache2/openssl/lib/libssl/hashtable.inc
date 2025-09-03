#	$NetBSD: hashtable.inc,v 1.1 2025/07/21 22:21:21 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/hashtable

HASHTABLE_SRCS += \
hashfunc.c \

SRCS += ${HASHTABLE_SRCS}

.for cryptosrc in ${HASHTABLE_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/hashtable ${HASHTABLECPPFLAGS}
.endfor
