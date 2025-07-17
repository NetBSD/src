#	$NetBSD: crypto_rc4.inc,v 1.1 2025/07/17 14:26:12 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/rc4

CRYPTO_RC4_SRCS += \

SRCS += ${CRYPTO_RC4_SRCS}

.for legacysrc in ${CRYPTO_RC4_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/crypto/rc4 ${CRYPTO_RC4CPPFLAGS}
.endfor
