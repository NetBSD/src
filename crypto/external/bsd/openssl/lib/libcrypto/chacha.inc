#	$NetBSD: chacha.inc,v 1.3 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/chacha

CHACHA_SRCS += \

SRCS += ${CHACHA_SRCS}

.for cryptosrc in ${CHACHA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/chacha ${CHACHACPPFLAGS}
.endfor
