#	$NetBSD: providers_implementations_storemgmt.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/storemgmt

PROVIDERS_IMPLEMENTATIONS_STOREMGMT_SRCS += \
file_store.c \
file_store_any2obj.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_STOREMGMT_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_STOREMGMT_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/storemgmt ${PROVIDERS_IMPLEMENTATIONS_STOREMGMTCPPFLAGS}
.endfor
