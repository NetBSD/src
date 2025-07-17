#	$NetBSD: providers_implementations_keymgmt.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/keymgmt

PROVIDERS_IMPLEMENTATIONS_KEYMGMT_SRCS += \
dh_kmgmt.c \
dsa_kmgmt.c \
ec_kmgmt.c \
ecx_kmgmt.c \
ml_dsa_kmgmt.c \
ml_kem_kmgmt.c \
mlx_kmgmt.c \
kdf_legacy_kmgmt.c \
mac_legacy_kmgmt.c \
rsa_kmgmt.c \
slh_dsa_kmgmt.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_KEYMGMT_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_KEYMGMT_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/keymgmt ${PROVIDERS_IMPLEMENTATIONS_KEYMGMTCPPFLAGS}
.endfor
