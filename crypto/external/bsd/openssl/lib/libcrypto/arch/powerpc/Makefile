#	$NetBSD: Makefile,v 1.4 2016/10/14 16:09:44 spz Exp $

.include "bsd.own.mk"

CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
XLATE=${OPENSSLSRC}/crypto/perlasm/ppc-xlate.pl

regen:
	for i in $$(find ${OPENSSLSRC} -name \*ppc\*.pl); do \
	    case $$i in \
	    *xlate.pl) ;; \
	    *sha512*) perl $$i 32 sha512; \
                      sed -e 's/bc	25,0,/bdnz+	/' \
                          -e 's/bclr	14,2/beqlr/' \
                          -e 's/bclr	6,2/bnelr/' \
                          -e 's/bclr	14,0/bltlr/' \
                          -e 's/bclr	12,0/bltlr/' \
                      < sha512 > $$(basename $$i .pl).S; \
                      perl $$i 32 sha256; \
                      sed -e 's/bc	25,0,/bdnz+	/' \
                          -e 's/bclr	14,2/beqlr/' \
                          -e 's/bclr	6,2/bnelr/' \
                          -e 's/bclr	14,0/bltlr/' \
                          -e 's/bclr	12,0/bltlr/' \
                      < sha256 > $$(basename $$i .pl | sed s/512/256/).S; \
                      rm -f sha512 sha256;; \
            *) perl $$i 32 | perl ${XLATE} linux32 | \
               sed -e 's/bc	25,0,/bdnz+	/' \
                   -e 's/bclr	14,2/beqlr/' \
                   -e 's/bclr	6,2/bnelr/' \
                   -e 's/bclr	14,0/bltlr/' \
                   -e 's/bclr	12,0/bltlr/' \
                   > $$(basename $$i .pl).S ;; \
	    esac; \
	done
