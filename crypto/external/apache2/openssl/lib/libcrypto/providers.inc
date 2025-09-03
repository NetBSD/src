#	$NetBSD: providers.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/providers ${OPENSSLSRC}/providers

PROVIDERS_SRCS += \
baseprov.c \
defltprov.c \
nullprov.c \
prov_running.c \


SRCS += ${PROVIDERS_SRCS}

.for cryptosrc in ${PROVIDERS_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/providers ${PROVIDERSCPPFLAGS}
.endfor
