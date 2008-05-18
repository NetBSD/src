#	$NetBSD: Makefile,v 1.1.2.2 2008/05/18 12:30:22 yamt Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*86.pl); do \
		perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i elf -fPIC \
		> $$(basename $$i .pl).S; \
	done
