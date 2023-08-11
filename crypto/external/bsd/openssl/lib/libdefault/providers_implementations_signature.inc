#	$NetBSD: providers_implementations_signature.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/signature

PROVIDERS_IMPLEMENTATIONS_SIGNATURE_SRCS += \
dsa_sig.c \
ecdsa_sig.c \
eddsa_sig.c \
mac_legacy_sig.c \
rsa_sig.c \
sm2_sig.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_SIGNATURE_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_SIGNATURE_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/signature ${PROVIDERS_IMPLEMENTATIONS_SIGNATURECPPFLAGS}
.endfor
