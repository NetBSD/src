#	$NetBSD: ocsp.inc,v 1.3 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/ocsp

OCSP_SRCS += \
ocsp_asn.c \
ocsp_cl.c \
ocsp_err.c \
ocsp_ext.c \
ocsp_http.c \
ocsp_lib.c \
ocsp_prn.c \
ocsp_srv.c \
ocsp_vfy.c \
v3_ocsp.c \

SRCS += ${OCSP_SRCS}

.for cryptosrc in ${OCSP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ocsp ${OCSPCPPFLAGS}
.endfor
