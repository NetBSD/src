#	$NetBSD: providers_common.inc,v 1.1.2.2 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/providers/common

PROVIDERS_COMMON_SRCS += \
provider_ctx.c \
provider_err.c \

SRCS += ${PROVIDERS_COMMON_SRCS}

.for commonsrc in ${PROVIDERS_COMMON_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/providers/common ${PROVIDERS_COMMONCPPFLAGS}
.endfor
