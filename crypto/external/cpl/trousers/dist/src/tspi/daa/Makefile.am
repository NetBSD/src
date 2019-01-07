bin_PROGRAMS=issuer_setup key_verification test test_tpm test_join test_sign
#todel

test_SOURCES=big_integer/bi_gmp.c big_integer/bi_openssl.c big_integer/bi.c utils/list.c big_integer/test/test.c big_integer/test/multi_exp.c
test_CFLAGS=-I../../include/daa -I../../include -DBI_DEBUG -g -DAPPID=\"BI\"
test_LDFLAGS=-lcrypto

issuer_setup_SOURCES = daa_issuer/issuer_setup.c
issuer_setup_CFLAGS=-I../../include/daa -I../../include -DBI_DEBUG -DAPPID=\"DAA_ISSUER_SETUP\"
issuer_setup_LDFLAGS=-lcrypto ../libtspi.la ../libdaa.la

key_verification_SOURCES=daa_issuer/key_verification.c
key_verification_CFLAGS=-I../../include/daa -I../../include -DBI_DEBUG -DAPPID=\"DAA_KEY_VERIFICATION\"
key_verification_LDFLAGS=-lcrypto ../libtspi.la ../libdaa.la

test_tpm_SOURCES = daa_platform/test.c
test_tpm_CFLAGS=-I../../include/daa -I../../include
test_tpm_LDFLAGS=-lcrypto ../libtspi.la ../libdaa.la

test_join_SOURCES = daa_platform/test_join.c
test_join_CFLAGS=-I../../include/daa -I../../include -DBI_DEBUG -DAPPID=\"DAA_JOIN\"
test_join_LDFLAGS=-lcrypto ../libtspi.la ../libdaa.la

test_sign_SOURCES = test_sign.c
test_sign_CFLAGS=-I../../include/daa -I../../include -DBI_DEBUG -DAPPID=\"DAA_JOIN\"
test_sign_LDFLAGS=-lcrypto ../libtspi.la ../libdaa.la

#todel_SOURCES = todel.c
#todel_CFLAGS=-I../../include/daa -I../../include -DBI_DEBUG -DAPPID=\"DAA_JOIN\"
#todel_LDFLAGS=-lcrypto ../libtspi.la ../libdaa.la 

