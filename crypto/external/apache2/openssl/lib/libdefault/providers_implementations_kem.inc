#	$NetBSD: providers_implementations_kem.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/kem

PROVIDERS_IMPLEMENTATIONS_KEM_SRCS += \
ec_kem.c \
ecx_kem.c \
kem_util.c \
ml_kem_kem.c \
mlx_kem.c \
rsa_kem.c \


SRCS += ${PROVIDERS_IMPLEMENTATIONS_KEM_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_KEM_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/kem ${PROVIDERS_IMPLEMENTATIONS_KEMCPPFLAGS}
.endfor
