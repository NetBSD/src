#	$NetBSD: ct.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/ct

CT_SRCS += \
ct_b64.c \
ct_err.c \
ct_log.c \
ct_oct.c \
ct_policy.c \
ct_prn.c \
ct_sct.c \
ct_sct_ctx.c \
ct_vfy.c \
ct_x509v3.c \

SRCS += ${CT_SRCS}

.for cryptosrc in ${CT_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ct ${CTCPPFLAGS}
.endfor
