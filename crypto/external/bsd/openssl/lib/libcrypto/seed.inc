#	$NetBSD: seed.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

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
