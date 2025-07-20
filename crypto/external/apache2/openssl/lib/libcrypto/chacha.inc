#	$NetBSD: chacha.inc,v 1.2 2025/07/20 00:32:10 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/chacha

CHACHA_SRCS ?= chacha_enc.c chacha_riscv.c

SRCS += ${CHACHA_SRCS}

.for cryptosrc in ${CHACHA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/chacha ${CHACHACPPFLAGS}
.endfor
