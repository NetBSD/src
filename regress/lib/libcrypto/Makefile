#	$NetBSD: Makefile,v 1.3 2003/07/14 13:26:30 itojun Exp $

.include <bsd.own.mk>

SUBDIR+=bf bn cast des dh dsa hmac lhash md2 md4 md5 rand rc2 \
	rc4 ripemd rsa sha sha1 x509v3
#SUBDIR+=conf threads

.if ${MKCRYPTO_IDEA} == "yes"
SUBDIR+=idea
.endif
.if ${MKCRYPTO_RC5} == "yes"
SUBDIR+=rc5
.endif
.if ${MKCRYPTO_MDC2} == "yes"
SUBDIR+=mdc2
.endif

.include <bsd.subdir.mk>
