#	$NetBSD: providers_implementations_asymciphers.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/asymciphers

PROVIDERS_IMPLEMENTATIONS_ASYMCIPHERS_SRCS += \
rsa_enc.c \
sm2_enc.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_ASYMCIPHERS_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_ASYMCIPHERS_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/asymciphers ${PROVIDERS_IMPLEMENTATIONS_ASYMCIPHERSCPPFLAGS}
.endfor
