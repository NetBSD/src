#	$NetBSD: chacha.inc,v 1.3 2025/07/20 00:36:08 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/chacha

CHACHA_SRCS ?= chacha_enc.c

SRCS += ${CHACHA_SRCS}

.for cryptosrc in ${CHACHA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/chacha ${CHACHACPPFLAGS}
.endfor
