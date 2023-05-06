#	$NetBSD: crmf.inc,v 1.1 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/crmf

CRMF_SRCS += \
crmf_asn.c \
crmf_err.c \
crmf_lib.c \
crmf_pbm.c \

SRCS += ${CRMF_SRCS}

.for cryptosrc in ${CRMF_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/crmf ${CRMFCPPFLAGS}
.endfor
