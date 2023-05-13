#	$NetBSD: ec.inc,v 1.12 2023/05/13 18:59:36 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/ec

EC_SRCS += \
curve25519.c \
ec2_oct.c \
ec2_smpl.c \
ec_ameth.c \
ec_asn1.c \
ec_backend.c \
ec_check.c \
ec_curve.c \
ec_cvt.c \
ec_deprecated.c \
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
ecp_oct.c \
ecp_smpl.c \
ecx_backend.c \
ecx_key.c \
ecx_meth.c \

.if defined(ECNI)
.if !defined(ECNONISTZ)
COPTS.ecp_nistz256.c+=-Wno-error=stack-protector
EC_SRCS+= ecp_nistz256.c
.endif
.if empty(ECCPPFLAGS:M*OPENSSL_NO_EC_NISTP_64_GCC_128*)
EC_SRCS+= ecp_nistp224.c ecp_nistp256.c ecp_nistp521.c ecp_nistputil.c
.endif
.endif

SRCS += ${EC_SRCS}

.for cryptosrc in ${EC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ec ${ECCPPFLAGS}
.endfor
