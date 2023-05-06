#	$NetBSD: hmac.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/hmac

HMAC_SRCS += \
hmac.c \

SRCS += ${HMAC_SRCS}

.for cryptosrc in ${HMAC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/hmac ${HMACCPPFLAGS}
.endfor
