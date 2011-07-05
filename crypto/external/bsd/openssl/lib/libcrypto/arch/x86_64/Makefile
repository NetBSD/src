#	$NetBSD: Makefile,v 1.3 2011/07/05 10:42:54 spz Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}.pl) \
		${OPENSSLSRC}/crypto/${MACHINE_ARCH}cpuid.pl ; do \
                perl $$i elf | sed -e 's/(%rip)/@GOTPCREL(%rip)/' \
		> $$(basename $$i .pl).S; \
	done
