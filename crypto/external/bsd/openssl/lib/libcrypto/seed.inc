#	$NetBSD: seed.inc,v 1.1.48.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/seed

SEED_SRCS += \
seed.c \
seed_cbc.c \
seed_cfb.c \
seed_ecb.c \
seed_ofb.c \

SRCS += ${SEED_SRCS}

.for cryptosrc in ${SEED_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/seed ${SEEDCPPFLAGS}
.endfor
