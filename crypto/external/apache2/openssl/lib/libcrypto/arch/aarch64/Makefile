#	$NetBSD: Makefile,v 1.2 2026/11/25 19:51:16 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*arm\*.pl) \
		$$(find ${OPENSSLSRC} -name \*aarch64\*.pl); do \
		case $$i in \
		(*/charmap.pl|*/arm-xlate.pl|*/*v4*|*/bsaes-armv7.pl);; \
		(*/sha512-armv8.pl) \
		perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i linux64 sha256-armv8.S; \
		perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i linux64 sha512-armv8.S;; \
		(*) perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i linux64 /dev/stdout \
		> $$(basename $$i .pl).S;; \
		esac; \
	done
