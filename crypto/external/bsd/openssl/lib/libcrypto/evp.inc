#	$NetBSD: evp.inc,v 1.11 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/evp

EVP_SRCS += \
asymcipher.c \
bio_b64.c \
bio_enc.c \
bio_md.c \
bio_ok.c \
c_allc.c \
c_alld.c \
cmeth_lib.c \
ctrl_params_translate.c \
dh_ctrl.c \
dh_support.c \
digest.c \
dsa_ctrl.c \
e_aes.c \
e_aes_cbc_hmac_sha1.c \
e_aes_cbc_hmac_sha256.c \
e_aria.c \
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
e_sm4.c \
e_xcbc_d.c \
ec_ctrl.c \
ec_support.c \
encode.c \
evp_cnf.c \
evp_enc.c \
evp_err.c \
evp_fetch.c \
evp_key.c \
evp_lib.c \
evp_pbe.c \
evp_pkey.c \
evp_rand.c \
evp_utils.c \
exchange.c \
kdf_lib.c \
kdf_meth.c \
kem.c \
keymgmt_lib.c \
keymgmt_meth.c \
legacy_blake2.c \
legacy_md4.c \
legacy_md5.c \
legacy_md5_sha1.c \
legacy_mdc2.c \
legacy_ripemd.c \
legacy_sha.c \
legacy_wp.c \
m_null.c \
m_sigver.c \
mac_lib.c \
mac_meth.c \
names.c \
p5_crpt.c \
p5_crpt2.c \
p_dec.c \
p_enc.c \
p_legacy.c \
p_lib.c \
p_open.c \
p_seal.c \
p_sign.c \
p_verify.c \
pbe_scrypt.c \
pmeth_check.c \
pmeth_gn.c \
pmeth_lib.c \
signature.c \

SRCS += ${EVP_SRCS}

.for cryptosrc in ${EVP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/evp ${EVPCPPFLAGS}
.endfor
