#	$NetBSD: providers_common.inc,v 1.1.2.2 2023/08/11 13:42:15 martin Exp $

.PATH:	${OPENSSLSRC}/providers/common

PROVIDERS_COMMON_SRCS += \
provider_util.c \

SRCS += ${PROVIDERS_COMMON_SRCS}

.for legacysrc in ${PROVIDERS_COMMON_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers/common ${PROVIDERS_COMMONCPPFLAGS}
.endfor
