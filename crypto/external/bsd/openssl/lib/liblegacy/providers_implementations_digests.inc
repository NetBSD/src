#	$NetBSD: providers_implementations_digests.inc,v 1.1 2023/05/07 16:22:10 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/digests

PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS += \
md4_prov.c \
mdc2_prov.c \
ripemd_prov.c \
wp_prov.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS}

.for legacysrc in ${PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers/implementations/digests ${PROVIDERS_IMPLEMENTATIONS_DIGESTSCPPFLAGS}
.endfor
