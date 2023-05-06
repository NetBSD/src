#	$NetBSD: providers_implementations_keymgmt.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/keymgmt

PROVIDERS_IMPLEMENTATIONS_KEYMGMT_SRCS += \
dh_kmgmt.c \
dsa_kmgmt.c \
ec_kmgmt.c \
ecx_kmgmt.c \
kdf_legacy_kmgmt.c \
mac_legacy_kmgmt.c \
rsa_kmgmt.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_KEYMGMT_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_KEYMGMT_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/keymgmt ${PROVIDERS_IMPLEMENTATIONS_KEYMGMTCPPFLAGS}
.endfor
