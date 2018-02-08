#	$NetBSD: ocsp.inc,v 1.2 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ocsp/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/ocsp


OCSP_SRCS = \
ocsp_asn.c \
ocsp_cl.c \
ocsp_err.c \
ocsp_ext.c \
ocsp_ht.c \
ocsp_lib.c \
ocsp_prn.c \
ocsp_srv.c \
ocsp_vfy.c \
v3_ocsp.c

SRCS += ${OCSP_SRCS}

.for cryptosrc in ${OCSP_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ocsp
.endfor
