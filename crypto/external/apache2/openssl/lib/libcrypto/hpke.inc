#	$NetBSD: hpke.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/hpke

HPKE_SRCS += \
hpke.c \
hpke_util.c \

SRCS += ${HPKE_SRCS}

.for cryptosrc in ${HPKE_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/hpke ${HPKECPPFLAGS}
.endfor
