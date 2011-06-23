#	$NetBSD: Makefile,v 1.1.4.1 2011/06/23 14:18:10 cherry Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*sparcv9\*.pl); do \
		j=$$(basename $$i .pl).S; \
		case $$j in \
		sha*) perl $$i > $$j;; \
		*) perl $$i -m64 > $$j;; \
		esac; \
	done
