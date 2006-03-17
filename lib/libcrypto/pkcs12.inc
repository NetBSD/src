#	$NetBSD: pkcs12.inc,v 1.5 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/pkcs12/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/pkcs12


PKCS12_SRCS = p12_add.c p12_asn.c p12_attr.c p12_crpt.c p12_crt.c p12_decr.c \
	p12_init.c p12_key.c p12_kiss.c p12_mutl.c\
	p12_utl.c p12_npas.c pk12err.c p12_p8d.c p12_p8e.c
SRCS += ${PKCS12_SRCS}

.for cryptosrc in ${PKCS12_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/pkcs12
.endfor
