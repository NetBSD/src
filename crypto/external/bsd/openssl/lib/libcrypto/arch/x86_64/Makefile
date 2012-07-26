#	$NetBSD: Makefile,v 1.4 2012/07/26 21:22:47 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}.pl) \
		${OPENSSLSRC}/crypto/${MACHINE_ARCH}cpuid.pl ; do \
                perl $$i elf | sed -e 's/\(OPENSSL[A-Za-z0-9_+]*\)(%rip)/\1@GOTPCREL(%rip)/' \
		> $$(basename $$i .pl).S; \
	done
