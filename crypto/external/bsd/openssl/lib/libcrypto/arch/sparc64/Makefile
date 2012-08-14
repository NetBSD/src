#	$NetBSD: Makefile,v 1.2.4.1 2012/08/14 07:57:07 riz Exp $

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
