# $NetBSD: Makefile,v 1.13 2018/02/22 14:24:03 martin Exp $

.include <bsd.own.mk>

SUBDIR+=bf bn cast conf des dh dsa ec ecdh ecdsa engine evp hmac \
	md2 md4 md5 rand rc2 rc4 ripemd rsa sha1 srp threads

SUBDIR+=idea mdc2
SUBDIR+=rc5

.if ${HAVE_OPENSSL} == 10
SUBDIR += lhash sha x509v3
.endif

TESTSDIR=	${TESTSBASE}/crypto/libcrypto

.if ${HAVE_OPENSSL} == 10
TESTS_SH=	t_certs
.endif
TESTS_SH+=	t_ciphers
TESTS_SH+=	t_hashes
TESTS_SH+=	t_libcrypto
TESTS_SH+=	t_pubkey

.include <bsd.test.mk>
