#	$NetBSD: Makefile,v 1.4 2000/10/04 07:15:07 itojun Exp $

VERSION=	0.9.5a
CRYPTODIST=	${.CURDIR}/../../../crypto/dist

manpages:
	for i in ${CRYPTODIST}/openssl/doc/apps/*.pod; do \
		fn=`basename $$i .pod`; \
		if [ "$$fn" = "config" ]; then \
			sec=5; \
			dst=openssl.cnf.$$sec; \
		else \
			sec=1; \
			dst=`basename $$i .pod`.$$sec; \
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

clean:
	-/bin/rm -f *.[0-9]
