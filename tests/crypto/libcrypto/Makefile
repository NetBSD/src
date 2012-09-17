# $NetBSD: Makefile,v 1.8.4.1 2012/09/17 19:03:11 riz Exp $

.include <bsd.own.mk>

.if ${MKCRYPTO} != "no"

SUBDIR+=bf bn cast conf des dh dsa ec ecdh ecdsa engine evp hmac lhash \
	md2 md4 md5 rand rc2 rc4 ripemd rsa sha sha1 srp threads x509v3 \
	idea mdc2

.if ${MKCRYPTO_RC5} != "no"
SUBDIR+=rc5
.endif

TESTSDIR=	${TESTSBASE}/crypto/libcrypto

TESTS_SH=	t_certs
TESTS_SH+=	t_ciphers
TESTS_SH+=	t_hashes
TESTS_SH+=	t_libcrypto
TESTS_SH+=	t_pubkey

.include <bsd.test.mk>

.else

NOPROG=

.include <bsd.prog.mk>

.endif
