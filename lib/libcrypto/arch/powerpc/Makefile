#	$NetBSD: Makefile,v 1.2.2.2 2008/05/18 12:30:22 yamt Exp $

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
