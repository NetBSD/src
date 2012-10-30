#	$NetBSD: Makefile,v 1.2.2.1 2012/10/30 18:47:32 yamt Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*sparcv9\*.pl); do \
		j=$$(basename $$i .pl).S; \
		case $$j in \
		ghash*|sha*) perl $$i > $$j;; \
		*) perl $$i -m64 > $$j;; \
		esac; \
	done
foo:
