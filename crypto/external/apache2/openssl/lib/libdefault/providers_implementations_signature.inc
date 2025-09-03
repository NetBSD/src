#	$NetBSD: providers_implementations_signature.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/signature

PROVIDERS_IMPLEMENTATIONS_SIGNATURE_SRCS += \
dsa_sig.c \
ecdsa_sig.c \
eddsa_sig.c \
mac_legacy_sig.c \
ml_dsa_sig.c \
rsa_sig.c \
slh_dsa_sig.c \
sm2_sig.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_SIGNATURE_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_SIGNATURE_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/signature ${PROVIDERS_IMPLEMENTATIONS_SIGNATURECPPFLAGS}
.endfor
