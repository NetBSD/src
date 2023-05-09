#	$NetBSD: Makefile,v 1.7 2023/05/09 17:21:17 christos Exp $

.include <bsd.own.mk>

BITS?=32
CRYPTODIST=${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
XLATE=${OPENSSLSRC}/crypto/perlasm/ppc-xlate.pl

regen:
	for i in $$(find ${OPENSSLSRC} -name \*ppc\*.pl); do \
	    case $$i in \
	    *xlate.pl) ;; \
	    *sha512*) perl $$i ${BITS} sha512 > sha512; \
                      sed -e 's/bc	25,0,/bdnz+	/' \
                          -e 's/bclr	14,2/beqlr/' \
                          -e 's/bclr	6,2/bnelr/' \
                          -e 's/bclr	14,0/bltlr/' \
                          -e 's/bclr	12,0/bltlr/' \
                      < sha512 > $$(basename $$i .pl).S; \
                      perl $$i ${BITS} sha256 > sha256; \
                      sed -e 's/bc	25,0,/bdnz+	/' \
                          -e 's/bclr	14,2/beqlr/' \
                          -e 's/bclr	6,2/bnelr/' \
                          -e 's/bclr	14,0/bltlr/' \
                          -e 's/bclr	12,0/bltlr/' \
                      < sha256 > $$(basename $$i .pl | sed -e s/512/256/).S; \
                      rm -f sha512 sha256;; \
	    *keccak1600-ppc64.pl) \
	    	if [ ${BITS} = 32 ]; then \
			continue; \
		fi; \
	        perl $$i ${BITS} | perl ${XLATE} linux${BITS} | \
                sed -e 's/bc	25,0,/bdnz+	/' \
                    -e 's/bclr	14,2/beqlr/' \
                    -e 's/bclr	6,2/bnelr/' \
                    -e 's/bclr	14,0/bltlr/' \
                    -e 's/bclr	12,0/bltlr/' \
                    > $$(basename $$i .pl).S ;; \
            *) perl $$i ${BITS} | perl ${XLATE} linux${BITS} | \
               sed -e 's/bc	25,0,/bdnz+	/' \
                   -e 's/bclr	14,2/beqlr/' \
                   -e 's/bclr	6,2/bnelr/' \
                   -e 's/bclr	14,0/bltlr/' \
                   -e 's/bclr	12,0/bltlr/' \
                   > $$(basename $$i .pl).S ;; \
	    esac; \
	done
