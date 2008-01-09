#	$NetBSD: evp.inc,v 1.7.10.1 2008/01/09 01:34:34 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/evp/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/evp


EVP_SRCS = encode.c digest.c evp_enc.c evp_key.c evp_acnf.c \
	e_des.c e_bf.c e_idea.c e_des3.c \
	e_rc4.c e_aes.c names.c \
	e_xcbc_d.c e_rc2.c e_cast.c e_rc5.c \
	m_null.c m_md2.c m_md4.c m_md5.c m_sha.c m_sha1.c \
	m_dss.c m_dss1.c m_mdc2.c m_ripemd.c \
	p_open.c p_seal.c p_sign.c p_verify.c p_lib.c p_enc.c p_dec.c \
	bio_md.c bio_b64.c bio_enc.c evp_err.c e_null.c \
	c_all.c c_allc.c c_alld.c evp_lib.c bio_ok.c \
	evp_pkey.c evp_pbe.c p5_crpt.c p5_crpt2.c m_ecdsa.c
SRCS += ${EVP_SRCS}

.for cryptosrc in ${EVP_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/evp
.endfor
