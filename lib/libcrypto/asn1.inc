#	$NetBSD: asn1.inc,v 1.2.2.2 2000/06/23 16:16:20 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/asn1/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/asn1

CFLAGS+=	-I${OPENSSLSRC}/crypto/asn1

SRCS+= a_object.c a_bitstr.c a_utctm.c a_gentm.c a_time.c a_int.c a_octet.c \
	a_print.c a_type.c a_set.c a_dup.c a_d2i_fp.c a_i2d_fp.c a_bmp.c \
	a_enum.c a_vis.c a_utf8.c a_sign.c a_digest.c a_verify.c \
	x_algor.c x_val.c x_pubkey.c x_sig.c x_req.c x_attrib.c \
	x_name.c x_cinf.c x_x509.c x_crl.c x_info.c x_spki.c nsseq.c \
	d2i_r_pr.c i2d_r_pr.c d2i_r_pu.c i2d_r_pu.c \
	d2i_s_pr.c i2d_s_pr.c d2i_s_pu.c i2d_s_pu.c \
	d2i_pu.c d2i_pr.c i2d_pu.c i2d_pr.c \
	t_req.c t_x509.c t_crl.c t_pkey.c \
	p7_i_s.c p7_signi.c p7_signd.c p7_recip.c p7_enc_c.c p7_evp.c \
	p7_dgst.c p7_s_e.c p7_enc.c p7_lib.c \
	f_int.c f_string.c i2d_dhp.c i2d_dsap.c d2i_dhp.c d2i_dsap.c n_pkey.c \
	f_enum.c a_hdr.c x_pkey.c a_bool.c x_exten.c \
	asn1_par.c asn1_lib.c asn1_err.c a_meth.c a_bytes.c \
	evp_asn1.c asn_pack.c p5_pbe.c p5_pbev2.c p8_pkey.c


