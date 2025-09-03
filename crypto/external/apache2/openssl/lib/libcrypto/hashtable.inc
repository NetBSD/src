#	$NetBSD: hashtable.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/hashtable

HASHTABLE_SRCS += \
hashfunc.c \
hashtable.c \

SRCS += ${HASHTABLE_SRCS}

.for cryptosrc in ${HASHTABLE_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/hashtable ${HASHTABLECPPFLAGS}
.endfor
