.PATH:	${.CURDIR}/man

MAN+=	ca.1 dsa.1 gendsa.1 rsa.1 genrsa.1 spkac.1 enc.1 speed.1 sess_id.1 \
	rand.1 passwd.1 smime.1 asn1parse.1 crl.1 openssl.1 dsaparam.1 \
	dhparam.1 version.1 req.1 nseq.1 s_server.1 ciphers.1 s_client.1 \
	dgst.1 verify.1 pkcs12.1 pkcs7.1 crl2pkcs7.1 pkcs8.1 x509.1 openssl.1
#MAN+=	CA.pl.1 

MAN+=	ERR_GET_LIB.3 RSA_sign_ASN1_OCTET_STRING.3 DSA_dup_DH.3 \
	OPENSSL_VERSION_NUMBER.3 sha.3 dsa.3 rsa.3 CRYPTO_set_ex_data.3 \
	hmac.3 RAND_add.3 BN_add.3 RAND_egd.3 ripemd.3 rand.3 BN_rand.3 \
	RAND_set_rand_method.3 DSA_set_method.3 RSA_set_method.3 DH_set_method.3 \
	BN_add_word.3 RAND_load_file.3 BN_generate_prime.3 BN_mod_inverse.3 \
	ERR_remove_state.3 DSA_size.3 RSA_size.3 DH_size.3 ERR_error_string.3 \
	dh.3 lhash.3 blowfish.3 BN_mod_mul_reciprocal.3 bn_internal.3 \
	bn.3 DSA_sign.3 RSA_sign.3 DSA_do_sign.3 BN_bn2bin.3 RSA_blinding_on.3 \
	BN_zero.3 BN_cmp.3 RAND_cleanup.3 buffer.3 ERR_clear_error.3 \
	SSL_get_error.3 ERR_get_error.3 ERR_put_error.3 err.3 threads.3 \
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
	RSA_padding_add_PKCS1_type_1.3 mdc2.3 rc4.3 md5.3 \
	crypto.3

MAN+=	ssl.4

MAN+=	openssl.cnf.5

MAN+=	des_modes.7
