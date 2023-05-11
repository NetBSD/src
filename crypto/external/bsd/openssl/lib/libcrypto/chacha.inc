#	$NetBSD: chacha.inc,v 1.4 2023/05/11 20:08:59 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/chacha

CHACHA_SRCS ?= chacha_enc.c

SRCS += ${CHACHA_SRCS}

.for cryptosrc in ${CHACHA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/chacha ${CHACHACPPFLAGS}
.endfor
