#	$NetBSD: lhash.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/lhash

LHASH_SRCS += \
lh_stats.c \
lhash.c \

SRCS += ${LHASH_SRCS}

.for cryptosrc in ${LHASH_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/lhash ${LHASHCPPFLAGS}
.endfor
