#	$NetBSD: providers_implementations_kem.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/kem

PROVIDERS_IMPLEMENTATIONS_KEM_SRCS += \
rsa_kem.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_KEM_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_KEM_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/kem ${PROVIDERS_IMPLEMENTATIONS_KEMCPPFLAGS}
.endfor
