#	$NetBSD: providers_implementations_asymciphers.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/asymciphers

PROVIDERS_IMPLEMENTATIONS_ASYMCIPHERS_SRCS += \
rsa_enc.c \
sm2_enc.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_ASYMCIPHERS_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_ASYMCIPHERS_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/asymciphers ${PROVIDERS_IMPLEMENTATIONS_ASYMCIPHERSCPPFLAGS}
.endfor
