#	$NetBSD: dh.inc,v 1.5 2018/09/23 13:33:04 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/dh/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/dh


DH_SRCS = \
dh_ameth.c \
dh_asn1.c \
dh_check.c \
dh_depr.c \
dh_err.c \
dh_gen.c \
dh_kdf.c \
dh_key.c \
dh_lib.c \
dh_meth.c \
dh_pmeth.c \
dh_prn.c \
dh_rfc5114.c \
dh_rfc7919.c

SRCS += ${DH_SRCS}

.for cryptosrc in ${DH_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/dh
.endfor
