#	$NetBSD: ec.inc,v 1.4 2007/12/09 22:44:16 adrianp Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ec/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/ec


EC_SRCS = ec_lib.c ecp_smpl.c ecp_mont.c ecp_nist.c ec_cvt.c ec_mult.c \
	ec_err.c ec_asn1.c ec_key.c ec_print.c ec_curve.c ec2_smpl.c \
	ec2_mult.c ec_check.c
SRCS += ${EC_SRCS}

.for cryptosrc in ${EC_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ec
.endfor
