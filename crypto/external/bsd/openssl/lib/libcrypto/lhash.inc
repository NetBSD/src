#	$NetBSD: lhash.inc,v 1.1.58.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/lhash

LHASH_SRCS += \
lh_stats.c \
lhash.c \

SRCS += ${LHASH_SRCS}

.for cryptosrc in ${LHASH_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/lhash ${LHASHCPPFLAGS}
.endfor
