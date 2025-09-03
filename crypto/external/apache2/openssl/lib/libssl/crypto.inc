#	$NetBSD: crypto.inc,v 1.1 2025/07/21 22:21:21 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto
.PATH.S:	${OPENSSLSRC}/crypto

CRYPTO_SRCS += \
arch.c \
ctype.c \
getenv.c \
packet.c \
quic_vlint.c \
time.c \

SRCS += ${CRYPTO_SRCS}

.for cryptosrc in ${CRYPTO_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/crypto ${CRYPTOCPPFLAGS}
.endfor

CRYPTOCPPFLAGS+=-DOPENSSL_NO_STATIC_ENGINE
