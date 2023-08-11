#	$NetBSD: chacha.inc,v 1.2.14.1 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/chacha

CHACHA_SRCS ?= chacha_enc.c

SRCS += ${CHACHA_SRCS}

.for cryptosrc in ${CHACHA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/chacha ${CHACHACPPFLAGS}
.endfor
