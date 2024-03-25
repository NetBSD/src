# $NetBSD: Makefile,v 1.14.2.1 2024/03/25 14:26:15 martin Exp $

.include <bsd.own.mk>

SUBDIR+=bf bn cast conf des dh dsa ec ecdh ecdsa engine evp hmac \
	md2 rc2 rc4 rsa srp threads

SUBDIR+=idea mdc2
SUBDIR+=rc5

.if ${HAVE_OPENSSL} == 10
SUBDIR += lhash sha x509v3
.endif

TESTSDIR=	${TESTSBASE}/crypto/libcrypto

TESTS_C+=		t_sha512trunc
DPADD.t_sha512trunc+=	${LIBCRYPTO}
LDADD.t_sha512trunc+=	-lcrypto

.if ${HAVE_OPENSSL} == 10
TESTS_SH=	t_certs
.endif
TESTS_SH+=	t_ciphers
TESTS_SH+=	t_hashes
TESTS_SH+=	t_libcrypto
TESTS_SH+=	t_pubkey

.include <bsd.test.mk>
