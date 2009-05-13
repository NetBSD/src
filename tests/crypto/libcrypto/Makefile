# $NetBSD: Makefile,v 1.3.2.2 2009/05/13 19:19:18 jym Exp $

.include <bsd.own.mk>

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

TESTS_SH=	t_libcrypto

CLEANFILES+=	t_libcrypto.sh
t_libcrypto.sh: t_libcrypto.awk t_libcrypto.in
	${TOOL_AWK} -f ${.CURDIR}/t_libcrypto.awk ${.CURDIR}/t_libcrypto.in \
	    >t_libcrypto.sh.tmp
	mv t_libcrypto.sh.tmp t_libcrypto.sh

.include <bsd.subdir.mk>
.include <bsd.test.mk>
