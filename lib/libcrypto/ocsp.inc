#	$NetBSD: ocsp.inc,v 1.1 2003/07/24 14:16:32 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ocsp/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ocsp

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ocsp

SRCS+=	ocsp_asn.c ocsp_ext.c ocsp_ht.c ocsp_lib.c ocsp_cl.c \
	ocsp_srv.c ocsp_prn.c ocsp_vfy.c ocsp_err.c
