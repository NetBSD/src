#	$NetBSD: siphash.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/siphash

SIPHASH_SRCS += \
siphash.c \

SRCS += ${SIPHASH_SRCS}

.for cryptosrc in ${SIPHASH_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/siphash ${SIPHASHCPPFLAGS}
.endfor
