#	$NetBSD: dh.inc,v 1.7 2007/12/09 22:44:15 adrianp Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/dh/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/dh


DH_SRCS = dh_asn1.c dh_gen.c dh_key.c dh_lib.c dh_check.c dh_err.c dh_depr.c
SRCS += ${DH_SRCS}

.for cryptosrc in ${DH_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/dh
.endfor
