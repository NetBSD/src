#	$NetBSD: Makefile,v 1.1 2018/03/03 00:20:57 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*arm\*.pl | grep -v charmap.pl); \
	do \
		case $$i in \
		(*/charmap.pl|*/arm-xlate.pl);; \
		(*) perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i linux /dev/stdout \
		> $$(basename $$i .pl).S;; \
		esac; \
	done
