#	$NetBSD: providers_implementations_macs.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

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
