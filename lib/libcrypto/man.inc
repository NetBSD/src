.PATH:	${.CURDIR}/man

MAN+=	openssl.1 openssl_asn1parse.1 openssl_ca.1 openssl_ciphers.1 \
	openssl_crl.1 openssl_crl2pkcs7.1 openssl_dgst.1 openssl_dhparam.1 \
	openssl_dsa.1 openssl_dsaparam.1 openssl_enc.1 openssl_gendsa.1 \
	openssl_genrsa.1 openssl_nseq.1 openssl_passwd.1 openssl_pkcs12.1 \
	openssl_pkcs7.1 openssl_pkcs8.1 openssl_rand.1 openssl_req.1 \
	openssl_rsa.1 openssl_rsautl.1 openssl_s_client.1 openssl_s_server.1 \
	openssl_sess_id.1 openssl_smime.1 openssl_speed.1 openssl_spkac.1 \
	openssl_verify.1 openssl_version.1 openssl_x509.1

#MAN+=	openssl_CA.pl.1

MAN+=	BIO_ctrl.3 BIO_f_base64.3 BIO_f_buffer.3 BIO_f_cipher.3 BIO_f_md.3 \
	BIO_f_null.3 BIO_f_ssl.3 BIO_find_type.3 BIO_new.3 BIO_new_bio_pair.3 \
	BIO_push.3 BIO_read.3 BIO_s_accept.3 BIO_s_bio.3 BIO_s_connect.3 \
	BIO_s_fd.3 BIO_s_file.3 BIO_s_mem.3 BIO_s_null.3 BIO_s_socket.3 \
	BIO_set_callback.3 BIO_should_retry.3 BN_CTX_new.3 BN_CTX_start.3 \
	BN_add.3 BN_add_word.3 BN_bn2bin.3 BN_cmp.3 BN_copy.3 BN_generate_prime.3 \
	BN_mod_inverse.3 BN_mod_mul_montgomery.3 BN_mod_mul_reciprocal.3 \
	BN_new.3 BN_num_bytes.3 BN_rand.3 BN_set_bit.3 BN_zero.3 \
	CRYPTO_set_ex_data.3 DH_generate_key.3 DH_generate_parameters.3 \
	DH_get_ex_new_index.3 DH_new.3 DH_set_method.3 DH_size.3 DSA_SIG_new.3 \
	DSA_do_sign.3 DSA_dup_DH.3 DSA_generate_key.3 DSA_generate_parameters.3 \
	DSA_get_ex_new_index.3 DSA_new.3 DSA_set_method.3 DSA_sign.3 \
	DSA_size.3 ERR_GET_LIB.3 ERR_clear_error.3 ERR_error_string.3 \
	ERR_get_error.3 ERR_load_crypto_strings.3 ERR_load_strings.3 \
	ERR_print_errors.3 ERR_put_error.3 ERR_remove_state.3 EVP_DigestInit.3 \
	EVP_EncryptInit.3 EVP_OpenInit.3 EVP_SealInit.3 EVP_SignInit.3 \
	EVP_VerifyInit.3 OPENSSL_VERSION_NUMBER.3 OpenSSL_add_all_algorithms.3 \
	RAND_add.3 RAND_bytes.3 RAND_cleanup.3 RAND_egd.3 RAND_load_file.3 \
	RAND_set_rand_method.3 RSA_blinding_on.3 RSA_check_key.3 \
	RSA_generate_key.3 RSA_get_ex_new_index.3 RSA_new.3 \
	RSA_padding_add_PKCS1_type_1.3 RSA_print.3 RSA_private_encrypt.3 \
	RSA_public_encrypt.3 RSA_set_method.3 RSA_sign.3 \
	RSA_sign_ASN1_OCTET_STRING.3 RSA_size.3 SSL_CIPHER_get_name.3 \
	SSL_CTX_add_extra_chain_cert.3 SSL_CTX_add_session.3 \
	SSL_CTX_flush_sessions.3 SSL_CTX_free.3 SSL_CTX_get_ex_new_index.3 \
	SSL_CTX_get_verify_mode.3 SSL_CTX_load_verify_locations.3 SSL_CTX_new.3 \
	SSL_CTX_sess_number.3 SSL_CTX_sess_set_cache_size.3 \
	SSL_CTX_sess_set_get_cb.3 SSL_CTX_sessions.3 SSL_CTX_set_cipher_list.3 \
	SSL_CTX_set_client_CA_list.3 SSL_CTX_set_default_passwd_cb.3 \
	SSL_CTX_set_mode.3 SSL_CTX_set_options.3 SSL_CTX_set_session_cache_mode.3 \
	SSL_CTX_set_session_id_context.3 SSL_CTX_set_ssl_version.3 \
	SSL_CTX_set_timeout.3 SSL_CTX_set_verify.3 SSL_CTX_use_certificate.3 \
	SSL_SESSION_free.3 SSL_SESSION_get_ex_new_index.3 SSL_SESSION_get_time.3 \
	SSL_accept.3 SSL_clear.3 SSL_connect.3 SSL_free.3 SSL_get_ciphers.3 \
	SSL_get_client_CA_list.3 SSL_get_current_cipher.3 SSL_get_error.3 \
	SSL_get_ex_data_X509_STORE_CTX_idx.3 SSL_get_ex_new_index.3 \
	SSL_get_fd.3 SSL_get_peer_cert_chain.3 SSL_get_peer_certificate.3 \
	SSL_get_rbio.3 SSL_get_session.3 SSL_get_verify_result.3 \
	SSL_get_version.3 SSL_library_init.3 SSL_load_client_CA_file.3 \
	SSL_new.3 SSL_pending.3 SSL_read.3 SSL_set_bio.3 SSL_set_connect_state.3 \
	SSL_set_fd.3 SSL_set_session.3 SSL_set_shutdown.3 SSL_set_verify_result.3 \
	SSL_shutdown.3 SSL_write.3 crypto.3 d2i_DHparams.3 d2i_RSAPublicKey.3 \
	d2i_SSL_SESSION.3 lh_stats.3 openssl_bio.3 openssl_blowfish.3 \
	openssl_bn.3 openssl_bn_internal.3 openssl_buffer.3 openssl_des.3 \
	openssl_dh.3 openssl_dsa.3 openssl_err.3 openssl_evp.3 openssl_hmac.3 \
	openssl_lhash.3 openssl_md5.3 openssl_mdc2.3 openssl_rand.3 \
	openssl_rc4.3 openssl_ripemd.3 openssl_rsa.3 openssl_sha.3 \
	openssl_threads.3 ssl.3
MAN+=	SSL_COMP_add_compression_method.3 SSL_CTX_ctrl.3 \
	SSL_CTX_set_cert_store.3 SSL_CTX_set_cert_verify_callback.3 \
	SSL_CTX_set_client_cert_cb.3 SSL_CTX_set_info_callback.3 \
	SSL_CTX_set_quiet_shutdown.3 SSL_CTX_set_tmp_dh_callback.3 \
	SSL_CTX_set_tmp_rsa_callback.3 SSL_alert_type_string.3 \
	SSL_get_SSL_CTX.3 SSL_get_default_timeout.3 \
	SSL_rstate_string.3 SSL_session_reused.3 SSL_state_string.3 \
	SSL_want.3

MAN+=	openssl.cnf.5

MAN+=	des_modes.7
