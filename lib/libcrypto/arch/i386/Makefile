#	$NetBSD: Makefile,v 1.1.4.2 2008/06/23 05:02:09 wrstuden Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*86.pl); do \
		perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i elf -fPIC \
		> $$(basename $$i .pl).S; \
	done
