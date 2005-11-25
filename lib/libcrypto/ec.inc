#	$NetBSD: ec.inc,v 1.2 2005/11/25 19:15:08 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ec/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ec

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ec

SRCS+=	ec_lib.c ecp_smpl.c ecp_mont.c ecp_nist.c ec_cvt.c ec_mult.c \
	ec_err.c ec_asn1.c ec_key.c ec_print.c ec_curve.c ec2_smpl.c \
	ec2_mult.c ec_check.c
