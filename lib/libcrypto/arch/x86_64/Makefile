#	$NetBSD: Makefile,v 1.1.2.2 2008/05/18 12:30:23 yamt Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*${MACHINE_ARCH}.pl); do \
		perl $$i | sed -e 's/(%rip)/@GOTPCREL(%rip)/' \
		> $$(basename $$i .pl).S; \
	done
