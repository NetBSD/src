#	$NetBSD: Makefile,v 1.1 2009/07/19 23:30:46 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

regen:
	for i in $$(find ${OPENSSLSRC} -name \*sparcv9\*.pl); do \
		j=$$(basename $$i .pl).S; \
		case $$j in \
		sha*) perl $$i > $$j;; \
		*) perl $$i -m64 > $$j;; \
		esac; \
	done
