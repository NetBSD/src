#	$NetBSD: Makefile,v 1.1 2000/10/04 05:34:15 itojun Exp $

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
		sec=`[ "$$fn" = "config" ] && echo 5 || echo 1`; \
		(cd `dirname $$i`; \
		 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
			`basename $$i`) > `basename $$i .pod`.$$sec; \
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
