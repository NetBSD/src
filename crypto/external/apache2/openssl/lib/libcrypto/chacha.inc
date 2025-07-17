#	$NetBSD: chacha.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/chacha

CHACHA_SRCS ?= chacha_enc.c

SRCS += ${CHACHA_SRCS}

.for cryptosrc in ${CHACHA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/chacha ${CHACHACPPFLAGS}
.endfor
