#	$NetBSD: Makefile,v 1.1.4.2 2008/06/23 05:02:10 wrstuden Exp $

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
