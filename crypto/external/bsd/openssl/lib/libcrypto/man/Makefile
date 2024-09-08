#	$NetBSD: Makefile,v 1.12 2024/09/08 13:08:25 christos Exp $

# the makefile is not, and should not be visited during normal build process.
# try to use latest pod2man, which comes with the latest perl.

.include <bsd.own.mk>
CRYPTODIST=	${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

OPENSSLDIST=${.CURDIR}/../../../openssl-3.0.15
VERSION!=	awk '/\#\#\#/ { print $$6; exit 0; };' ${OPENSSLDIST}/CHANGES.md


make-manpages:
	for sec in 1 3 5 7; do \
	for i in ${OPENSSLDIST}/doc/man$$sec/*.pod; do \
		fn=`basename $$i .pod`; \
		if [ "$$fn" = "config" ]; then \
		    fn=openssl-config; \
		fi; \
		case $$sec in \
		1) if [ "$$fn" = "openssl" ]; then \
			dst=$$fn.$$sec; \
		   else \
			case $$fn in \
			openssl*) dst=$$fn.$$sec;; \
			*) dst=openssl-$$fn.$$sec;; \
			esac; \
		   fi;; \
		*) dst=$$fn.$$sec;; \
		esac; \
		(cd `dirname $$i`; \
		 pod2man --section=$$sec --center=OpenSSL --release=${VERSION} \
			`basename $$i`) | \
		sed -e 's/[[:space:]]*$$//' | \
		perl libcrypto.pl > $$dst; \
	done \
	done

clean:
	-/bin/rm -f *.[0-9]
