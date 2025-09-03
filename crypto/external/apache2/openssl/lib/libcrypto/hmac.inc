#	$NetBSD: hmac.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/hmac

HMAC_SRCS += \
hmac.c \

SRCS += ${HMAC_SRCS}

.for cryptosrc in ${HMAC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/hmac ${HMACCPPFLAGS}
.endfor
