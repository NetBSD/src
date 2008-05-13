#	$NetBSD: Makefile,v 1.1 2008/05/13 15:35:40 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
XLATE=${OPENSSLSRC}/crypto/perlasm/ppc-xlate.pl

regen:
	for i in $$(find ${OPENSSLSRC} -name \*ppc\*.pl); do \
	    case $$i in \
	    *xlate.pl) ;; \
	    *64*) ;; \
	    *) perl $$i 32 | perl ${XLATE} linux32 > $$(basename $$i .pl).S ;; \
	    esac; \
	done
