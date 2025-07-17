#	$NetBSD: ess.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/ess

ESS_SRCS += \
ess_asn1.c \
ess_err.c \
ess_lib.c \

SRCS += ${ESS_SRCS}

.for cryptosrc in ${ESS_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ess ${ESSCPPFLAGS}
.endfor
