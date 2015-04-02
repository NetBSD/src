#	$NetBSD: Makefile,v 1.2.12.2 2015/04/02 18:04:14 snj Exp $

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
	       sed -e 's/bc	25,0,/bdnz+	/' \
		   -e 's/bclr	14,2/beqlr/' > $$(basename $$i .pl).S ;; \
	    esac; \
	done
