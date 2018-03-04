#	$NetBSD: Makefile,v 1.1 2018/03/04 00:59:44 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	# poly is mips64 only
	for i in $$(find ${OPENSSLSRC} -name \*mips\*.pl); \
	do \
		case $$i in \
		(*poly*);; \
		(*) perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i linux /dev/stdout \
		> $$(basename $$i .pl).S;; \
		esac; \
	done
