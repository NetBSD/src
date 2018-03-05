#	$NetBSD: Makefile,v 1.2 2018/03/05 16:43:50 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*mips\*.pl); \
	do \
		case $$i in \
		(*poly*) perl -I${OPENSSLSRC}/crypto/perlasm \
			    -I${OPENSSLSRC}/crypto/bn/asm $$i 64 /dev/stdout \
			    > $$(basename $$i .pl)64.S;; \
		(*) perl -I${OPENSSLSRC}/crypto/perlasm \
			-I${OPENSSLSRC}/crypto/bn/asm $$i o32 /dev/stdout \
			> $$(basename $$i .pl).S; \
		    perl -I${OPENSSLSRC}/crypto/perlasm \
			-I${OPENSSLSRC}/crypto/bn/asm $$i 64 /dev/stdout \
			> $$(basename $$i .pl)64.S;; \
		esac; \
	done
