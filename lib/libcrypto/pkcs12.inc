#	$NetBSD: pkcs12.inc,v 1.1.1.1.2.1 2000/06/22 06:55:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/pkcs12/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/pkcs12

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/pkcs12

SRCS+= p12_add.c p12_attr.c p12_bags.c p12_crpt.c p12_crt.c p12_decr.c \
	p12_init.c p12_key.c p12_kiss.c p12_lib.c p12_mac.c p12_mutl.c\
	p12_sbag.c p12_utl.c pk12err.c


