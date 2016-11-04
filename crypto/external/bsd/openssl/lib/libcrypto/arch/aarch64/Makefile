#	$NetBSD: Makefile,v 1.1.2.2 2016/11/04 14:42:36 pgoyette Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in ${OPENSSLSRC}/crypto/aes/asm/aesv8-armx.pl \
		 ${OPENSSLSRC}/crypto/modes/asm/ghashv8-armx.pl \
		 ${OPENSSLSRC}/crypto/sha/asm/sha1-armv8.pl; do \
		j=$$(basename $$i .pl).S; \
		perl $$i > $$j; \
	done
