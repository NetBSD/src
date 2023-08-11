#	$NetBSD: siphash.inc,v 1.1.14.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/siphash

SIPHASH_SRCS += \
siphash.c \

SRCS += ${SIPHASH_SRCS}

.for cryptosrc in ${SIPHASH_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/siphash ${SIPHASHCPPFLAGS}
.endfor
