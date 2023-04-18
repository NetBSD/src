#	$NetBSD: Makefile,v 1.1.1.2 2023/04/18 14:19:04 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*arm\*.pl); do \
		case $$i in \
		(*/charmap.pl|*/arm-xlate.pl|*/*v4*|*/*v7*);; \
		(*) perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i linux /dev/stdout \
		> $$(basename $$i .pl).S;; \
		esac; \
	done
