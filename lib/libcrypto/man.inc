.PATH:	${.CURDIR}/man

MAN+=	openssl_ca.1 openssl_dsa.1 openssl_gendsa.1 openssl_rsa.1 \
	openssl_genrsa.1 openssl_spkac.1 openssl_enc.1 openssl_speed.1 \
	openssl_sess_id.1 openssl_rand.1 openssl_passwd.1 openssl_smime.1 \
	openssl_asn1parse.1 openssl_crl.1 openssl_dsaparam.1 \
	openssl_dhparam.1 openssl_version.1 openssl_req.1 openssl_nseq.1 \
	openssl_s_server.1 openssl_ciphers.1 openssl_s_client.1 openssl_dgst.1 \
	openssl_verify.1 openssl_pkcs12.1 openssl_pkcs7.1 openssl_crl2pkcs7.1 \
	openssl_pkcs8.1 openssl_x509.1
MAN1+=	openssl.1
#MAN+=	CA.pl.1 

MAN+=	ERR_GET_LIB.3 RSA_sign_ASN1_OCTET_STRING.3 DSA_dup_DH.3 \
	OPENSSL_VERSION_NUMBER.3 sha.3 dsa.3 rsa.3 CRYPTO_set_ex_data.3 \
	hmac.3 RAND_add.3 BN_add.3 RAND_egd.3 ripemd.3 RAND_seed.3 BN_rand.3 \
	RAND_set_rand_method.3 DSA_set_method.3 RSA_set_method.3 DH_set_method.3 \
	BN_add_word.3 RAND_load_file.3 BN_generate_prime.3 BN_mod_inverse.3 \
	ERR_remove_state.3 DSA_size.3 RSA_size.3 DH_size.3 ERR_error_string.3 \
	dh.3 lhash.3 blowfish.3 BN_mod_mul_reciprocal.3 bn_internal.3 \
	bn.3 DSA_sign.3 RSA_sign.3 DSA_do_sign.3 BN_bn2bin.3 RSA_blinding_on.3 \
	BN_zero.3 BN_cmp.3 RAND_cleanup.3 buffer.3 ERR_clear_error.3 \
	SSL_get_error.3 ERR_get_error.3 ERR_put_error.3 openssl_err.3 threads.3 \
	des.3 RAND_bytes.3 BN_num_bytes.3 ERR_load_strings.3 \
	ERR_load_crypto_strings.3 d2i_DHparams.3 OpenSSL_add_all_algorithms.3 \
	DSA_generate_parameters.3 DH_generate_parameters.3 ERR_print_errors.3 \
	lh_stats.3 BN_set_bit.3 EVP_SealInit.3 EVP_OpenInit.3 EVP_SignInit.3 \
	EVP_EncryptInit.3 EVP_DigestInit.3 EVP_VerifyInit.3 RSA_print.3 \
	RSA_public_encrypt.3 RSA_private_encrypt.3 BN_CTX_start.3 DSA_new.3 \
	RSA_new.3 DSA_SIG_new.3 DH_new.3 BN_new.3 BN_CTX_new.3 \
	DSA_get_ex_new_index.3 RSA_get_ex_new_index.3 DH_get_ex_new_index.3 \
	d2i_RSAPublicKey.3 DSA_generate_key.3 RSA_generate_key.3 \
	DH_generate_key.3 RSA_check_key.3 BN_copy.3 BN_mod_mul_montgomery.3 \
	RSA_padding_add_PKCS1_type_1.3 mdc2.3 rc4.3 MD5_Init.3 \
	crypto.3 ssl.3

MAN+=	openssl.cnf.5

MAN+=	des_modes.7
