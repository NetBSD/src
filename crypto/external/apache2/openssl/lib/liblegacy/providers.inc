#	$NetBSD: providers.inc,v 1.2 2025/07/21 13:10:23 christos Exp $

.PATH:	${OPENSSLSRC}/providers

PROVIDERS_SRCS += \
legacyprov.c \
prov_running.c \

SRCS += ${PROVIDERS_SRCS}

.for legacysrc in ${PROVIDERS_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers ${PROVIDERSCPPFLAGS}
.endfor
