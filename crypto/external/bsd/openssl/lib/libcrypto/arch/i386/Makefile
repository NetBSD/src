#	$NetBSD: Makefile,v 1.10 2018/02/10 05:51:06 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*86.pl) \
		  ${OPENSSLSRC}/crypto/x86cpuid.pl; do \
		perl -I${OPENSSLSRC}/crypto/perlasm \
		-I${OPENSSLSRC}/crypto/bn/asm $$i elf /dev/stdout \
		| sed -e 's,^\.file.*$$,#include <machine/asm.h>,' \
			-e 's/	call	OPENSSL_cpuid_setup/	PIC_PROLOGUE!	call	PIC_PLT(OPENSSL_cpuid_setup)!	PIC_EPILOGUE/' \
			-e 's/	leal	DES_SPtrans,%edx/	PIC_PROLOGUE!	leal	PIC_GOT(DES_SPtrans),%edx!	PIC_EPOLOGUE/' \
			-e 's/	leal	OPENSSL_ia32cap_P,%eax/	PIC_PROLOGUE!	leal	PIC_GOT(OPENSSL_ia32cap_P),%eax!	PIC_EPOLOGUE/' \
			| tr '!' '\n' \
		> $$(basename $$i .pl).S; \
	done
