#	$NetBSD: cms.inc,v 1.6 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/cms

CMS_SRCS += \
cms_asn1.c \
cms_att.c \
cms_cd.c \
cms_dd.c \
cms_dh.c \
cms_ec.c \
cms_enc.c \
cms_env.c \
cms_err.c \
cms_ess.c \
cms_io.c \
cms_kari.c \
cms_lib.c \
cms_pwri.c \
cms_rsa.c \
cms_sd.c \
cms_smime.c \

SRCS += ${CMS_SRCS}

.for cryptosrc in ${CMS_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/cms ${CMSCPPFLAGS}
.endfor
