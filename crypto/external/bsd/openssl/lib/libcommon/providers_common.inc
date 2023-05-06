#	$NetBSD: providers_common.inc,v 1.1 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/providers/common

PROVIDERS_COMMON_SRCS += \
provider_ctx.c \
provider_err.c \

SRCS += ${PROVIDERS_COMMON_SRCS}

.for commonsrc in ${PROVIDERS_COMMON_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/providers/common ${PROVIDERS_COMMONCPPFLAGS}
.endfor
