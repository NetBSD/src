#	$NetBSD: providers_implementations_digests.inc,v 1.1.2.2 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/digests

PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS += \
digestcommon.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS}

.for commonsrc in ${PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/providers/implementations/digests ${PROVIDERS_IMPLEMENTATIONS_DIGESTSCPPFLAGS}
.endfor
