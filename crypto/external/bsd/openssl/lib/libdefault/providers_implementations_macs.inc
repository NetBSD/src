#	$NetBSD: providers_implementations_macs.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/macs

PROVIDERS_IMPLEMENTATIONS_MACS_SRCS += \
blake2b_mac.c \
blake2s_mac.c \
cmac_prov.c \
gmac_prov.c \
hmac_prov.c \
kmac_prov.c \
poly1305_prov.c \
siphash_prov.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_MACS_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_MACS_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/macs ${PROVIDERS_IMPLEMENTATIONS_MACSCPPFLAGS}
.endfor
