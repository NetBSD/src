#	$NetBSD: providers_implementations_rands.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/rands

PROVIDERS_IMPLEMENTATIONS_RANDS_SRCS += \
drbg.c \
drbg_ctr.c \
drbg_hash.c \
drbg_hmac.c \
seed_src.c \
seed_src_jitter.c \
test_rng.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_RANDS_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_RANDS_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/rands ${PROVIDERS_IMPLEMENTATIONS_RANDSCPPFLAGS}
.endfor
