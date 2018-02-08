#	$NetBSD: evp.inc,v 1.7 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/evp/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/evp


EVP_SRCS = \
bio_b64.c \
bio_enc.c \
bio_md.c \
bio_ok.c \
c_allc.c \
c_alld.c \
cmeth_lib.c \
digest.c \
e_aes.c \
e_aes_cbc_hmac_sha1.c \
e_aes_cbc_hmac_sha256.c \
e_bf.c \
e_camellia.c \
e_cast.c \
e_chacha20_poly1305.c \
e_des.c \
e_des3.c \
e_idea.c \
e_null.c \
e_old.c \
e_rc2.c \
e_rc4.c \
e_rc4_hmac_md5.c \
e_rc5.c \
e_seed.c \
e_xcbc_d.c \
encode.c \
evp_cnf.c \
evp_enc.c \
evp_err.c \
evp_key.c \
evp_lib.c \
evp_pbe.c \
evp_pkey.c \
m_md2.c \
m_md4.c \
m_md5.c \
m_md5_sha1.c \
m_mdc2.c \
m_null.c \
m_ripemd.c \
m_sha1.c \
m_sigver.c \
m_wp.c \
names.c \
p5_crpt.c \
p5_crpt2.c \
p_dec.c \
p_enc.c \
p_lib.c \
p_open.c \
p_seal.c \
p_sign.c \
p_verify.c \
pmeth_fn.c \
pmeth_gn.c \
pmeth_lib.c \
scrypt.c

SRCS += ${EVP_SRCS}

.for cryptosrc in ${EVP_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/evp
.endfor

EVP_AES_SRCS = e_aes.c

# XXX Should do this too, but it requires some other asm changes.
#EVP_AES_SRCS += e_aes_cbc_hmac_sha1.c

.for cryptosrc in ${EVP_AES_SRCS}
CPPFLAGS.${cryptosrc} += ${AESCPPFLAGS}
.endfor

CPPFLAGS.e_aes.c += -I${OPENSSLSRC}/crypto/modes
