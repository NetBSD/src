#	$NetBSD: providers.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/providers ${OPENSSLSRC}/providers

PROVIDERS_SRCS += \
baseprov.c \
defltprov.c \
nullprov.c \
prov_running.c \


SRCS += ${PROVIDERS_SRCS}

.for cryptosrc in ${PROVIDERS_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/providers ${PROVIDERSCPPFLAGS}
.endfor
