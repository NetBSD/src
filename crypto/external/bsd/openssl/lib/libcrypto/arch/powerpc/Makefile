#	$NetBSD: Makefile,v 1.1.4.1 2011/06/23 14:18:10 cherry Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
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
