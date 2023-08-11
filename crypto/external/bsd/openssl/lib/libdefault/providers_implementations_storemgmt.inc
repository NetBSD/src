#	$NetBSD: providers_implementations_storemgmt.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/storemgmt

PROVIDERS_IMPLEMENTATIONS_STOREMGMT_SRCS += \
file_store.c \
file_store_any2obj.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_STOREMGMT_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_STOREMGMT_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/storemgmt ${PROVIDERS_IMPLEMENTATIONS_STOREMGMTCPPFLAGS}
.endfor
