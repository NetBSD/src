# $NetBSD: Makefile,v 1.4 2023/05/13 17:46:09 riastradh Exp $

.include <bsd.own.mk>
HELPER_NAME=	evp_test
HELPER_DIR=	evp

.if ${HAVE_OPENSSL} <= 11
FILES+= \
evpcase.txt \
evpciph.txt \
evpdigest.txt \
evpencod.txt \
evpkdf.txt \
evpmac.txt \
evppbe.txt \
evppkey.txt \
evppkey_ecc.txt
.else
FILES+= \
evpciph_aes_ccm_cavs.txt \
evpciph_aes_common.txt \
evpciph_aes_cts.txt \
evpciph_aes_ocb.txt \
evpciph_aes_siv.txt \
evpciph_aes_stitched.txt \
evpciph_aes_wrap.txt \
evpciph_aria.txt \
evpciph_bf.txt \
evpciph_camellia.txt \
evpciph_camellia_cts.txt \
evpciph_cast5.txt \
evpciph_chacha.txt \
evpciph_des.txt \
evpciph_des3_common.txt \
evpciph_idea.txt \
evpciph_rc2.txt \
evpciph_rc4.txt \
evpciph_rc4_stitched.txt \
evpciph_rc5.txt \
evpciph_seed.txt \
evpciph_sm4.txt \
evpencod.txt \
evpkdf_hkdf.txt \
evpkdf_krb5.txt \
evpkdf_pbkdf1.txt \
evpkdf_pbkdf2.txt \
evpkdf_scrypt.txt \
evpkdf_ss.txt \
evpkdf_ssh.txt \
evpkdf_tls11_prf.txt \
evpkdf_tls12_prf.txt \
evpkdf_tls13_kdf.txt \
evpkdf_x942.txt \
evpkdf_x942_des.txt \
evpkdf_x963.txt \
evpmac_blake.txt \
evpmac_cmac_des.txt \
evpmac_common.txt \
evpmac_poly1305.txt \
evpmac_siphash.txt \
evpmac_sm3.txt \
evpmd_blake.txt \
evpmd_md.txt \
evpmd_mdc2.txt \
evpmd_ripemd.txt \
evpmd_sha.txt \
evpmd_sm3.txt \
evpmd_whirlpool.txt \
evppbe_pbkdf2.txt \
evppbe_pkcs12.txt \
evppbe_scrypt.txt \
evppkey_brainpool.txt \
evppkey_dh.txt \
evppkey_dsa.txt \
evppkey_ecc.txt \
evppkey_ecdh.txt \
evppkey_ecdsa.txt \
evppkey_ecx.txt \
evppkey_ffdhe.txt \
evppkey_kas.txt \
evppkey_kdf_hkdf.txt \
evppkey_kdf_scrypt.txt \
evppkey_kdf_tls1_prf.txt \
evppkey_mismatch.txt \
evppkey_rsa.txt \
evppkey_rsa_common.txt \
evppkey_sm2.txt \
evprand.txt
COPTS.evp_test.c+=	-Wno-missing-field-initializers
.endif

.include <bsd.init.mk>
.PATH: ${OPENSSLSRC}/test/recipes/30-test_evp_data
