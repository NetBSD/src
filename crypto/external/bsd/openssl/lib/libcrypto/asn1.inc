#	$NetBSD: asn1.inc,v 1.3 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/asn1/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/asn1


ASN1_SRCS = \
a_bitstr.c \
a_d2i_fp.c \
a_digest.c \
a_dup.c \
a_gentm.c \
a_i2d_fp.c \
a_int.c \
a_mbstr.c \
a_object.c \
a_octet.c \
a_print.c \
a_sign.c \
a_strex.c \
a_strnid.c \
a_time.c \
a_type.c \
a_utctm.c \
a_utf8.c \
a_verify.c \
ameth_lib.c \
asn1_err.c \
asn1_gen.c \
asn1_lib.c \
asn1_par.c \
asn_mime.c \
asn_moid.c \
asn_mstbl.c \
asn_pack.c \
bio_asn1.c \
bio_ndef.c \
d2i_pr.c \
d2i_pu.c \
evp_asn1.c \
f_int.c \
f_string.c \
i2d_pr.c \
i2d_pu.c \
n_pkey.c \
nsseq.c \
p5_pbe.c \
p5_pbev2.c \
p5_scrypt.c \
p8_pkey.c \
t_bitst.c \
t_pkey.c \
t_spki.c \
tasn_dec.c \
tasn_enc.c \
tasn_fre.c \
tasn_new.c \
tasn_prn.c \
tasn_scn.c \
tasn_typ.c \
tasn_utl.c \
x_algor.c \
x_bignum.c \
x_info.c \
x_int64.c \
x_long.c \
x_pkey.c \
x_sig.c \
x_spki.c \
x_val.c

SRCS += ${ASN1_SRCS}

.for cryptosrc in ${ASN1_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/asn1
.endfor
