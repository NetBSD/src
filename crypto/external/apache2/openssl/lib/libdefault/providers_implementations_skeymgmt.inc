#	$NetBSD: providers_implementations_skeymgmt.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/skeymgmt

PROVIDERS_IMPLEMENTATIONS_SKEYMGMT_SRCS += \
aes_skmgmt.c \
generic.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_SKEYMGMT_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_SKEYMGMT_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/skeymgmt ${PROVIDERS_IMPLEMENTATIONS_SKEYMGMTCPPFLAGS}
.endfor
