#	$NetBSD: providers_common.inc,v 1.1 2025/07/17 14:25:23 christos Exp $

.PATH:	${OPENSSLSRC}/providers/common

PROVIDERS_COMMON_SRCS += \
provider_ctx.c \
provider_err.c \

SRCS += ${PROVIDERS_COMMON_SRCS}

.for commonsrc in ${PROVIDERS_COMMON_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/providers/common ${PROVIDERS_COMMONCPPFLAGS}
.endfor
