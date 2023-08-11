#	$NetBSD: providers.inc,v 1.1.2.2 2023/08/11 13:42:15 martin Exp $

.PATH:	${OPENSSLSRC}/providers

PROVIDERS_SRCS += \
prov_running.c \

SRCS += ${PROVIDERS_SRCS}

.for legacysrc in ${PROVIDERS_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers ${PROVIDERSCPPFLAGS}
.endfor
