#	$NetBSD: modes.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/modes

MODES_SRCS += \
cbc128.c \
ccm128.c \
cfb128.c \
ctr128.c \
cts128.c \
gcm128.c \
ocb128.c \
ofb128.c \
siv128.c \
wrap128.c \
xts128.c \
xts128gb.c \

SRCS += ${MODES_SRCS}

.for cryptosrc in ${MODES_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/modes ${MODESCPPFLAGS}
.endfor
