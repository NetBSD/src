#	$NetBSD: providers_implementations_rands.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/rands

PROVIDERS_IMPLEMENTATIONS_RANDS_SRCS += \
crngt.c \
drbg.c \
drbg_ctr.c \
drbg_hash.c \
drbg_hmac.c \
seed_src.c \
test_rng.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_RANDS_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_RANDS_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/rands ${PROVIDERS_IMPLEMENTATIONS_RANDSCPPFLAGS}
.endfor
