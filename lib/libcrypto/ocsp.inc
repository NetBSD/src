#	$NetBSD: ocsp.inc,v 1.3 2007/12/09 22:44:19 adrianp Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ocsp/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/ocsp


OCSP_SRCS = ocsp_asn.c ocsp_ext.c ocsp_ht.c ocsp_lib.c ocsp_cl.c \
	ocsp_srv.c ocsp_prn.c ocsp_vfy.c ocsp_err.c
SRCS += ${OCSP_SRCS}

.for cryptosrc in ${OCSP_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ocsp
.endfor
