#	$NetBSD: Makefile,v 1.1 2009/07/19 23:30:47 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}.pl); do \
		perl $$i | sed -e 's/(%rip)/@GOTPCREL(%rip)/' \
		> $$(basename $$i .pl).S; \
	done
