#	$NetBSD: krb5.inc,v 1.3 2007/12/09 22:44:17 adrianp Exp $
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
