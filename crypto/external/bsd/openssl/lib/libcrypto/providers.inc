#	$NetBSD: providers.inc,v 1.1.2.2 2023/08/11 13:41:55 martin Exp $

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
