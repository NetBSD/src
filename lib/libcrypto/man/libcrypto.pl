print '.\\"	$NetBSD: libcrypto.pl,v 1.1 2001/04/12 10:45:46 itojun Exp $' . "\n";
print '.\\"' . "\n";
while (<>) {
	next if (/\$RCSfile/);
	next if (/\$Log/);

	if (/\.SH "SYNOPSIS"/i) {
		print '.SH "LIBRARY"' . "\n";
		print 'libcrypto, -lcrypto' . "\n";
	}

	s/\bCA.pl\(1\)/openssl_CA.pl(1)/g;
	s/\basn1parse\(1\)/openssl_asn1parse(1)/g;
	s/\bca\(1\)/openssl_ca(1)/g;
	s/\bciphers\(1\)/openssl_ciphers(1)/g;
	s/\bcrl\(1\)/openssl_crl(1)/g;
	s/\bcrl2pkcs7\(1\)/openssl_crl2pkcs7(1)/g;
	s/\bdgst\(1\)/openssl_dgst(1)/g;
	s/\bdhparam\(1\)/openssl_dhparam(1)/g;
	s/\bdsa\(1\)/openssl_dsa(1)/g;
	s/\bdsaparam\(1\)/openssl_dsaparam(1)/g;
	s/\benc\(1\)/openssl_enc(1)/g;
	s/\bgendsa\(1\)/openssl_gendsa(1)/g;
	s/\bgenrsa\(1\)/openssl_genrsa(1)/g;
	s/\bnseq\(1\)/openssl_nseq(1)/g;
	s/\bpasswd\(1\)/openssl_passwd(1)/g;
	s/\bpkcs12\(1\)/openssl_pkcs12(1)/g;
	s/\bpkcs7\(1\)/openssl_pkcs7(1)/g;
	s/\bpkcs8\(1\)/openssl_pkcs8(1)/g;
	s/\brand\(1\)/openssl_rand(1)/g;
	s/\breq\(1\)/openssl_req(1)/g;
	s/\brsa\(1\)/openssl_rsa(1)/g;
	s/\brsautl\(1\)/openssl_rsautl(1)/g;
	s/\bs_client\(1\)/openssl_s_client(1)/g;
	s/\bs_server\(1\)/openssl_s_server(1)/g;
	s/\bsess_id\(1\)/openssl_sess_id(1)/g;
	s/\bsmime\(1\)/openssl_smime(1)/g;
	s/\bspeed\(1\)/openssl_speed(1)/g;
	s/\bspkac\(1\)/openssl_spkac(1)/g;
	s/\bverify\(1\)/openssl_verify(1)/g;
	s/\bversion\(1\)/openssl_version(1)/g;
	s/\bx509\(1\)/openssl_x509(1)/g;
	s/\bbio\(3\)/openssl_bio(3)/g;
	s/\bblowfish\(3\)/openssl_blowfish(3)/g;
	s/\bbn\(3\)/openssl_bn(3)/g;
	s/\bbn_internal\(3\)/openssl_bn_internal(3)/g;
	s/\bbuffer\(3\)/openssl_buffer(3)/g;
	s/\bdes\(3\)/openssl_des(3)/g;
	s/\bdh\(3\)/openssl_dh(3)/g;
	s/\bdsa\(3\)/openssl_dsa(3)/g;
	s/\berr\(3\)/openssl_err(3)/g;
	s/\bevp\(3\)/openssl_evp(3)/g;
	s/\bhmac\(3\)/openssl_hmac(3)/g;
	s/\blhash\(3\)/openssl_lhash(3)/g;
	s/\bmd5\(3\)/openssl_md5(3)/g;
	s/\bmdc2\(3\)/openssl_mdc2(3)/g;
	s/\brand\(3\)/openssl_rand(3)/g;
	s/\brc4\(3\)/openssl_rc4(3)/g;
	s/\bripemd\(3\)/openssl_ripemd(3)/g;
	s/\brsa\(3\)/openssl_rsa(3)/g;
	s/\bsha\(3\)/openssl_sha(3)/g;
	s/\bthreads\(3\)/openssl_threads(3)/g;
	s/\bconfig\(5\)/openssl.cnf(5)/g;

	print;
}
