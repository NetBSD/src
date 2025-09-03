#	$NetBSD: providers_common.inc,v 1.1 2025/07/17 14:26:12 christos Exp $

.PATH:	${OPENSSLSRC}/providers/common

PROVIDERS_COMMON_SRCS += \
provider_util.c \

SRCS += ${PROVIDERS_COMMON_SRCS}

.for legacysrc in ${PROVIDERS_COMMON_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers/common ${PROVIDERS_COMMONCPPFLAGS}
.endfor
