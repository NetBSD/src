#	$NetBSD: hmac.inc,v 1.1.58.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/hmac

HMAC_SRCS += \
hmac.c \

SRCS += ${HMAC_SRCS}

.for cryptosrc in ${HMAC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/hmac ${HMACCPPFLAGS}
.endfor
