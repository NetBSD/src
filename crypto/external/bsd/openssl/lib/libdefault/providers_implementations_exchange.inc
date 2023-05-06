#	$NetBSD: providers_implementations_exchange.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/exchange

PROVIDERS_IMPLEMENTATIONS_EXCHANGE_SRCS += \
dh_exch.c \
ecdh_exch.c \
ecx_exch.c \
kdf_exch.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_EXCHANGE_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_EXCHANGE_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/exchange ${PROVIDERS_IMPLEMENTATIONS_EXCHANGECPPFLAGS}
.endfor
