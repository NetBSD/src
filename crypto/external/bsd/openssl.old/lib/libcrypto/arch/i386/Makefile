#	$NetBSD: Makefile,v 1.1.1.2 2023/04/18 14:19:06 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*86.pl) \
		  ${OPENSSLSRC}/crypto/x86cpuid.pl; do \
		(echo '#include <machine/asm.h>' && \
		perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i elf -fPIC -DOPENSSL_IA32_SSE2 /dev/stdout) \
		| sed -e 's/	call	OPENSSL_cpuid_setup/	PIC_PROLOGUE!	call	PIC_PLT(OPENSSL_cpuid_setup)!	PIC_EPILOGUE/' | tr '!' '\n' \
		> $$(basename $$i .pl).S; \
	done
