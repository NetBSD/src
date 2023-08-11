#	$NetBSD: dh.inc,v 1.5.10.1 2023/08/11 13:41:55 martin Exp $

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
