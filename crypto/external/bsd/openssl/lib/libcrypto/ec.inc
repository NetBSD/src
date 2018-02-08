#	$NetBSD: ec.inc,v 1.4 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ec/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/ec


EC_SRCS += \
curve25519.c \
ec2_mult.c \
ec2_oct.c \
ec2_smpl.c \
ec_ameth.c \
ec_asn1.c \
ec_check.c \
ec_curve.c \
ec_cvt.c \
ec_err.c \
ec_key.c \
ec_kmeth.c \
ec_lib.c \
ec_mult.c \
ec_oct.c \
ec_pmeth.c \
ec_print.c \
ecdh_kdf.c \
ecdh_ossl.c \
ecdsa_ossl.c \
ecdsa_sign.c \
ecdsa_vrf.c \
eck_prn.c \
ecp_mont.c \
ecp_nist.c \
ecp_nistp224.c \
ecp_nistp256.c \
ecp_nistp521.c \
ecp_nistputil.c \
ecp_nistz256.c \
ecp_oct.c \
ecp_smpl.c \
ecx_meth.c

SRCS += ${EC_SRCS}

.for cryptosrc in ${EC_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ec
.endfor
