#	$NetBSD: Makefile,v 1.2 2000/10/04 05:39:08 itojun Exp $

VERSION=	0.9.5a
CRYPTODIST=	${.CURDIR}/../../../crypto/dist

manpages:
	for i in ${CRYPTODIST}/openssl/doc/*.pod; do \
		fn=`basename $$i .pod`; \
		sec=4; \
		(cd `dirname $$i`; \
		 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
			`basename $$i`) > `basename $$i .pod`.$$sec; \
	done
	for i in ${CRYPTODIST}/openssl/doc/apps/*.pod; do \
		fn=`basename $$i .pod`; \
		if [ "$$fn" = "config" ]; then \
			sec=5; \
			(cd `dirname $$i`; \
			 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
				`basename $$i`) > openssl.cnf.$$sec; \
		else \
			sec=1; \
			(cd `dirname $$i`; \
			 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
				`basename $$i`) > `basename $$i .pod`.$$sec; \
		fi \
	done
	for i in ${CRYPTODIST}/openssl/doc/crypto/*.pod \
	    ${CRYPTODIST}/openssl/doc/ssl/*.pod; do \
		fn=`basename $$i .pod`; \
		if [ "$$fn" = "des_modes" ]; then \
			sec=7; \
			(cd `dirname $$i`; \
			 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
				`basename $$i`) > `basename $$i .pod`.$$sec; \
		else \
			sec=3; \
			(cd `dirname $$i`; \
			 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
				`basename $$i`) | \
			 sed -f libcrypto.sed > `basename $$i .pod`.$$sec; \
		fi \
	done

clean:
	-/bin/rm -f *.[0-9]
