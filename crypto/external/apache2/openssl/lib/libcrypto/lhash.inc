#	$NetBSD: lhash.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/lhash

LHASH_SRCS += \
lh_stats.c \
lhash.c \

SRCS += ${LHASH_SRCS}

.for cryptosrc in ${LHASH_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/lhash ${LHASHCPPFLAGS}
.endfor
