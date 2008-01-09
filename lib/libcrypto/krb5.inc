#	$NetBSD: krb5.inc,v 1.2.10.1 2008/01/09 01:34:35 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/krb5/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/krb5


KRB5_SRCS = krb5_asn.c
SRCS += ${KRB5_SRCS}

.for cryptosrc in ${KRB5_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/krb5
.endfor
