#	$NetBSD: providers.inc,v 1.1 2023/05/07 16:22:10 christos Exp $

.PATH:	${OPENSSLSRC}/providers

PROVIDERS_SRCS += \
prov_running.c \

SRCS += ${PROVIDERS_SRCS}

.for legacysrc in ${PROVIDERS_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers ${PROVIDERSCPPFLAGS}
.endfor
