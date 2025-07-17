#	$NetBSD: providers_implementations_digests.inc,v 1.1 2025/07/17 14:25:23 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/digests

PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS += \
digestcommon.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS}

.for commonsrc in ${PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/providers/implementations/digests ${PROVIDERS_IMPLEMENTATIONS_DIGESTSCPPFLAGS}
.endfor
