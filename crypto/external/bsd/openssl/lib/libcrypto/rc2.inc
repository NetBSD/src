#	$NetBSD: rc2.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/rc2

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
