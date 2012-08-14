#	$NetBSD: Makefile,v 1.3.4.1 2012/08/14 07:57:07 riz Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}.pl) \
		${OPENSSLSRC}/crypto/${MACHINE_ARCH}cpuid.pl ; do \
                (echo "#include <machine/asm.h>"; perl $$i elf | sed \
		    -e 's/\(OPENSSL[A-Za-z0-9_+]*\)(%rip)/\1@GOTPCREL(%rip)/' \
		    -e 's/.hidden	OPENSSL_cpuid_setup/.globl	OPENSSL_cpuid_setup/' \
		    -e 's/call	OPENSSL_cpuid_setup/call	PIC_PLT(OPENSSL_cpuid_setup)/') \
		> $$(basename $$i .pl).S; \
	done
