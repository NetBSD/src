#	$NetBSD: Makefile,v 1.1 2025/07/17 14:25:27 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*arm\*.pl); do \
		case $$i in \
		(*/charmap.pl|*/arm-xlate.pl|*/*v4*|*/bsaes-armv7.pl);; \
		(*) perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i linux64 /dev/stdout \
		> $$(basename $$i .pl).S;; \
		esac; \
	done
