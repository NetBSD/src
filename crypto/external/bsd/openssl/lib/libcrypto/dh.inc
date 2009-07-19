#	$NetBSD: dh.inc,v 1.1 2009/07/19 23:30:43 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/dh/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/dh


DH_SRCS = dh_asn1.c dh_gen.c dh_key.c dh_lib.c dh_check.c dh_err.c dh_depr.c \
    dh_ameth.c dh_pmeth.c
SRCS += ${DH_SRCS}

.for cryptosrc in ${DH_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/dh
.endfor
