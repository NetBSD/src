#	$NetBSD: Makefile,v 1.5 2012/07/28 19:21:57 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*86.pl) \
		  ${OPENSSLSRC}/crypto/x86cpuid.pl; do \
		perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i elf -fPIC \
		| sed -e 's,^\.file.*$$,#include <machine/asm.h>,' \
			-e 's/OPENSSL_cpuid_setup/OPENSSL_cpuid_setup@PLT/' \
		> $$(basename $$i .pl).S; \
	done
