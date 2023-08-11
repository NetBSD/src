#	$NetBSD: crypto.inc,v 1.1.2.2 2023/08/11 13:42:15 martin Exp $

.PATH:	${OPENSSLSRC}/crypto

CRYPTO_SRCS += \
cpuid.c \
ctype.c \

SRCS += ${CRYPTO_SRCS}

.for legacysrc in ${CRYPTO_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/crypto ${CRYPTOCPPFLAGS}
.endfor
