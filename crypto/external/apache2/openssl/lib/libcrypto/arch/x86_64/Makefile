#	$NetBSD: Makefile,v 1.1 2025/07/17 14:25:37 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

.if make(regen) && ${HAVE_LLVM:U} == "yes"
CC+= -fno-integrated-as
.endif

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}.pl) \
		$$(find ${OPENSSLSRC} -name \*avx2\*.pl) \
		$$(find ${OPENSSLSRC} -name \*avx512\*.pl) \
		$$(find ${OPENSSLSRC} -name \*avxifma\*.pl) \
		${OPENSSLSRC}/crypto/${MACHINE_ARCH}cpuid.pl ; do \
		j=$$(basename $$i .pl).S; \
		case $$j in \
		(keccak*) \
		    perl $$i $$j;; \
		(sha512-x86_64.S) \
		    perl $$i sha256-x86_64.S; perl $$i sha512-x86_64.S;; \
		(*) \
		    (echo "#include <machine/asm.h>"; CC=${CC:Q} perl $$i elf | sed \
			-e 's/.hidden	OPENSSL_cpuid_setup/.globl	OPENSSL_cpuid_setup/' \
			-e 's/call	OPENSSL_cpuid_setup/call	PIC_PLT(OPENSSL_cpuid_setup)/' \
			-e 's/rdseedq/rdseed/') > $$j; \
		    if [ -f elf ]; then echo $$i; fi;; \
		esac; \
	done
