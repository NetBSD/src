#	$NetBSD: Makefile,v 1.6 2001/04/12 07:48:05 itojun Exp $

# the makefile is not, and should not be visited during normal build process.

VERSION=	0.9.6a
CRYPTODIST=	${.CURDIR}/../../../crypto/dist

manpages:
	for i in ${CRYPTODIST}/openssl/doc/apps/*.pod; do \
		fn=`basename $$i .pod`; \
		if [ "$$fn" = "config" ]; then \
			sec=5; \
			dst=openssl.cnf.$$sec; \
		elif [ "$$fn" = "openssl" ]; then \
			sec=1; \
			dst=`basename $$i .pod`.$$sec; \
		else \
			sec=1; \
			dst=openssl_`basename $$i .pod`.$$sec; \
		fi; \
		(cd `dirname $$i`; \
		 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
			`basename $$i`) | sed -e '/$$RCSfile/d' -e '/$$Log/d' > $$dst; \
	done
	for i in ${CRYPTODIST}/openssl/doc/crypto/*.pod \
	    ${CRYPTODIST}/openssl/doc/ssl/*.pod; do \
		fn=`basename $$i .pod`; \
		if [ "$$fn" = "des_modes" ]; then \
			sec=7; \
			(cd `dirname $$i`; \
			 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
				`basename $$i`) | sed -e '/$$RCSfile/d' -e '/$$Log/d' > `basename $$i .pod`.$$sec; \
		else \
			sec=3; \
			(cd `dirname $$i`; \
			 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
				`basename $$i`) | \
			 sed -f libcrypto.sed | \
			 sed -e '/$$RCSfile/d' -e '/$$Log/d' > `basename $$i .pod`.$$sec; \
		fi \
	done
	mv md5.3 MD5_Init.3
	mv err.3 openssl_err.3
	mv rand.3 RAND_seed.3

clean:
	-/bin/rm -f *.[0-9]
