#	$NetBSD: siphash.inc,v 1.1 2025/07/21 22:21:21 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/siphash

SIPHASH_SRCS += \
siphash.c \

SRCS += ${SIPHASH_SRCS}

.for cryptosrc in ${SIPHASH_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/siphash ${SIPHASHCPPFLAGS}
.endfor
