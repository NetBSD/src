#	$NetBSD: providers_implementations_kem.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/kem

PROVIDERS_IMPLEMENTATIONS_KEM_SRCS += \
rsa_kem.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_KEM_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_KEM_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/kem ${PROVIDERS_IMPLEMENTATIONS_KEMCPPFLAGS}
.endfor
