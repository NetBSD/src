#	$NetBSD: Makefile,v 1.4 2023/10/25 17:17:51 christos Exp $

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
