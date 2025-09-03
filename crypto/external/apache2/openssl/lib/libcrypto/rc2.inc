#	$NetBSD: rc2.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/rc2

RC2_SRCS += \
rc2_cbc.c \
rc2_ecb.c \
rc2_skey.c \
rc2cfb64.c \
rc2ofb64.c \

SRCS += ${RC2_SRCS}

.for cryptosrc in ${RC2_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/rc2 ${RC2CPPFLAGS}
.endfor
