#	$NetBSD: Makefile,v 1.1 2000/10/03 15:13:22 itojun Exp $

.include <bsd.own.mk>

SUBDIR+=bf bn cast des dh dsa hmac lhash md2 md5 mdc2 rand rc2 \
	rc4 ripemd rsa sha sha1 x509v3
#SUBDIR+=conf threads

.if ${MKCRYPTO_IDEA} == "yes"
SUBDIR+=idea
.endif
.if ${MKCRYPTO_RC5} == "yes"
SUBDIR+=rc5
.endif

.include <bsd.subdir.mk>
