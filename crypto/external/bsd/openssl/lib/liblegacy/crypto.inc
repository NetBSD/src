#	$NetBSD: crypto.inc,v 1.1 2023/05/07 16:22:10 christos Exp $

.PATH:	${OPENSSLSRC}/crypto

CRYPTO_SRCS += \
cpuid.c \
ctype.c \

SRCS += ${CRYPTO_SRCS}

.for legacysrc in ${CRYPTO_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/crypto ${CRYPTOCPPFLAGS}
.endfor
