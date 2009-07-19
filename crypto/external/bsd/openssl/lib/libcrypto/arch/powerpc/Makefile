#	$NetBSD: Makefile,v 1.1 2009/07/19 23:30:46 christos Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
XLATE=${OPENSSLSRC}/crypto/perlasm/ppc-xlate.pl

regen:
	for i in $$(find ${OPENSSLSRC} -name \*ppc\*.pl); do \
	    case $$i in \
	    *xlate.pl) ;; \
	    *64*) ;; \
	    *) perl $$i 32 | perl ${XLATE} linux32 | \
	       sed -e 's/bc	25,0,/bdnz+	/' > $$(basename $$i .pl).S ;; \
	    esac; \
	done
