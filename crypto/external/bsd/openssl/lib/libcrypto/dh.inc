#	$NetBSD: dh.inc,v 1.6 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/dh

DH_SRCS += \
dh_ameth.c \
dh_asn1.c \
dh_backend.c \
dh_check.c \
dh_depr.c \
dh_err.c \
dh_gen.c \
dh_group_params.c \
dh_kdf.c \
dh_key.c \
dh_lib.c \
dh_meth.c \
dh_pmeth.c \
dh_prn.c \
dh_rfc5114.c \

SRCS += ${DH_SRCS}

.for cryptosrc in ${DH_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/dh ${DHCPPFLAGS}
.endfor
