#	$NetBSD: Makefile,v 1.1 2008/05/10 20:14:20 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}.pl); do \
		perl $$i | sed -e 's/(%rip)/@GOTPCREL(%rip)/' \
		> $$(basename $$i .pl).S; \
	done
