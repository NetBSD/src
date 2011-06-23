#	$NetBSD: Makefile,v 1.1.4.1 2011/06/23 14:18:10 cherry Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}.pl); do \
		perl $$i | sed -e 's/(%rip)/@GOTPCREL(%rip)/' \
		> $$(basename $$i .pl).S; \
	done
