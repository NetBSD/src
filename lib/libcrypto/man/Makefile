#	$NetBSD: Makefile,v 1.7 2001/04/12 10:45:37 itojun Exp $

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
			`basename $$i`) | \
		perl libcrypto.pl >$$dst; \
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
			perl libcrypto.pl >`basename $$i .pod`.$$sec; \
		fi \
	done
	for i in bio blowfish bn bn_internal buffer des dh dsa err evp hmac \
	lhash md5 mdc2 rand rc4 ripemd rsa sha threads; do \
		mv $$i.3 openssl_$$i.3; \
	done

clean:
	-/bin/rm -f *.[0-9]
