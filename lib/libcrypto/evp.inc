#	$NetBSD: evp.inc,v 1.1.1.1.2.2 2000/07/17 05:59:59 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/evp/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/evp

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/evp

SRCS+=	encode.c digest.c evp_enc.c evp_key.c \
	e_ecb_d.c e_cbc_d.c e_cfb_d.c e_ofb_d.c \
	e_ecb_i.c e_cbc_i.c e_cfb_i.c e_ofb_i.c \
	e_ecb_3d.c e_cbc_3d.c e_rc4.c names.c \
	e_cfb_3d.c e_ofb_3d.c e_xcbc_d.c \
	e_ecb_r2.c e_cbc_r2.c e_cfb_r2.c e_ofb_r2.c \
	e_ecb_bf.c e_cbc_bf.c e_cfb_bf.c e_ofb_bf.c \
	e_ecb_c.c e_cbc_c.c e_cfb_c.c e_ofb_c.c \
	e_ecb_r5.c e_cbc_r5.c e_cfb_r5.c e_ofb_r5.c \
	m_null.c m_md2.c m_md5.c m_sha.c m_sha1.c m_dss.c m_dss1.c m_mdc2.c \
	m_ripemd.c \
	p_open.c p_seal.c p_sign.c p_verify.c p_lib.c p_enc.c p_dec.c \
	bio_md.c bio_b64.c bio_enc.c evp_err.c e_null.c \
	c_all.c c_allc.c c_alld.c evp_lib.c bio_ok.c \
	evp_pkey.c evp_pbe.c p5_crpt.c p5_crpt2.c


