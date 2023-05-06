#	$NetBSD: providers_implementations_ciphers.inc,v 1.1 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/ciphers

PROVIDERS_IMPLEMENTATIONS_CIPHERS_SRCS += \
ciphercommon.c \
ciphercommon_block.c \
ciphercommon_ccm.c \
ciphercommon_ccm_hw.c \
ciphercommon_gcm.c \
ciphercommon_gcm_hw.c \
ciphercommon_hw.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_CIPHERS_SRCS}

.for commonsrc in ${PROVIDERS_IMPLEMENTATIONS_CIPHERS_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/providers/implementations/ciphers ${PROVIDERS_IMPLEMENTATIONS_CIPHERSCPPFLAGS}
.endfor
