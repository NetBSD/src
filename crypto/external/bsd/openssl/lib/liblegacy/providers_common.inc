#	$NetBSD: providers_common.inc,v 1.1 2023/05/07 16:22:10 christos Exp $

.PATH:	${OPENSSLSRC}/providers/common

PROVIDERS_COMMON_SRCS += \
provider_util.c \

SRCS += ${PROVIDERS_COMMON_SRCS}

.for legacysrc in ${PROVIDERS_COMMON_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers/common ${PROVIDERS_COMMONCPPFLAGS}
.endfor
