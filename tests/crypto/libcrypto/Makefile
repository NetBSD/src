# $NetBSD: Makefile,v 1.6 2010/07/13 21:13:21 jmmv Exp $

.include <bsd.own.mk>

.if ${MKCRYPTO} != "no"

SUBDIR+=bf bn cast conf des dh dsa ec ecdh ecdsa engine hmac lhash \
	md2 md4 md5 rand rc2 rc4 ripemd rsa sha sha1 threads x509v3

.if ${MKCRYPTO_IDEA} != "no"
SUBDIR+=idea
.endif
.if ${MKCRYPTO_RC5} != "no"
SUBDIR+=rc5
.endif
.if ${MKCRYPTO_MDC2} != "no"
SUBDIR+=mdc2
.endif

TESTSDIR=	${TESTSBASE}/crypto/libcrypto

TESTS_SH=	t_certs
TESTS_SH+=	t_ciphers
TESTS_SH+=	t_hashes
TESTS_SH+=	t_libcrypto
TESTS_SH+=	t_pubkey

.include <bsd.test.mk>

.endif
